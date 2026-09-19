"""Independent concrete P-Code execution core.

The design is intentionally data-oriented rather than a copy of Ghidra's Java
executor hierarchy.  Ghidra supplies P-Code and the reference semantics; this
module owns concrete state, one-op stepping, control flow, and instrumentation.
The integer rules follow the operations in
``Ghidra/Framework/Emulation/src/main/java/ghidra/pcode/opbehavior`` and the
state boundaries in ``ghidra/pcode/exec/PcodeExecutorState``.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable, Iterable, Mapping, Protocol


def _mask(bits: int) -> int:
    """Return an all-ones mask for ``bits`` bits, rejecting invalid widths."""
    if bits <= 0:
        raise ValueError(f"P-Code values must have a positive width, got {bits}")
    return (1 << bits) - 1


def _signed(value: int, size: int) -> int:
    """Interpret a masked ``size``-byte value using two's-complement rules."""
    bits = size * 8
    value &= _mask(bits)
    sign = 1 << (bits - 1)
    return value - (1 << bits) if value & sign else value


def _truncate_dividend(value: int, size: int) -> int:
    """Normalize a host integer to the bit width of a P-Code input."""
    return value & _mask(size * 8)


@dataclass(frozen=True)
class Varnode:
    """Describe a P-Code value location or constant.

    ``space`` is normally ``constant``, ``register``, ``unique``, ``ram``, or
    ``stack``.  ``offset`` is the address-space offset.  Register and unique
    values are byte-addressed storage locations, while constants use ``offset``
    as their literal value.  ``name`` is optional display metadata supplied by
    the PyGhidra adapter.
    """

    space: str
    offset: int
    size: int
    name: str | None = None
    space_id: int | None = None

    @property
    def is_constant(self) -> bool:
        """Return whether this varnode is an immediate constant."""
        return self.space == "constant"

    def format(self) -> str:
        """Return a concise human-readable varnode description."""
        if self.is_constant:
            return f"0x{self.offset:x}:{self.size}B"
        label = self.name or f"{self.space}[0x{self.offset:x}]"
        return f"{label}:{self.size}B"


@dataclass(frozen=True)
class PcodeOp:
    """Represent one P-Code instruction independent of Java runtime objects."""

    mnemonic: str
    inputs: tuple[Varnode, ...] = ()
    output: Varnode | None = None
    address: int | None = None
    sequence: int | None = None

    def format(self) -> str:
        """Render this operation in a stable form suitable for reports/errors."""
        operands = ", ".join(node.format() for node in self.inputs)
        if self.output is None:
            return f"{self.mnemonic} {operands}".rstrip()
        return f"{self.mnemonic} {operands} -> {self.output.format()}"


class PcodeObserver(Protocol):
    """Optional event interface implemented by tracers and analyzers."""

    def before_op(self, emulator: "PcodeEmulator", op: PcodeOp, step: int, state: "StateSnapshot") -> None:
        """Observe the immutable state immediately before one P-Code operation."""

    def after_op(
        self,
        emulator: "PcodeEmulator",
        op: PcodeOp,
        step: int,
        before: "StateSnapshot",
        after: "StateSnapshot",
    ) -> None:
        """Observe the immutable state immediately after one P-Code operation."""

    def on_memory_read(self, space: str, address: int, size: int, value: int) -> None:
        """Observe a concrete memory read."""

    def on_memory_write(self, space: str, address: int, size: int, value: int) -> None:
        """Observe a concrete memory write."""

    def on_register_read(self, varnode: Varnode, value: int) -> None:
        """Observe a register or unique-space read."""

    def on_register_write(self, varnode: Varnode, value: int) -> None:
        """Observe a register or unique-space write."""

    def on_branch(self, op: PcodeOp, target: int, taken: bool) -> None:
        """Observe a branch decision and its resolved P-Code index/address."""

    def on_call(self, op: PcodeOp, target: int) -> None:
        """Observe a concrete call target."""

    def on_return(self, op: PcodeOp) -> None:
        """Observe a function return."""


@dataclass(frozen=True)
class StateSnapshot:
    """Immutable sparse snapshot used for per-operation diffing."""

    storage: Mapping[tuple[str, int, int], int]
    labels: Mapping[tuple[str, int, int], str]
    memory: Mapping[str, Mapping[int, int]]
    pc: int
    halted: bool


class MachineState:
    """Concrete registers, unique temporaries, arbitrary memory, and control state.

    Storage is byte-addressed for registers and unique values.  This preserves
    overlapping x86 register varnodes and allows every varnode width to be
    masked at its boundary, matching Ghidra's byte-oriented executor state.
    Memory is sparse and segmented by address-space name, so large unmapped
    regions do not require allocation.
    """

    def __init__(self, *, little_endian: bool = True) -> None:
        """Create an empty state with the selected byte order."""
        self.little_endian = little_endian
        self._storage: dict[tuple[str, int], int] = {}
        self._memory: dict[str, dict[int, int]] = {}
        self.pc = 0
        self.halted = False
        self._descriptions: dict[tuple[str, int, int], str] = {}
        self._locations: set[tuple[str, int, int]] = set()

    def describe(self, varnode: Varnode) -> None:
        """Register optional display metadata for a storage varnode."""
        if not varnode.is_constant and varnode.name:
            self._descriptions[(varnode.space, varnode.offset, varnode.size)] = varnode.name
        if not varnode.is_constant and varnode.space in {"register", "unique"}:
            self._locations.add((varnode.space, varnode.offset, varnode.size))

    def read(self, varnode: Varnode) -> int:
        """Read and width-mask a varnode, returning a concrete unsigned value."""
        if varnode.is_constant:
            value = varnode.offset
        elif varnode.space in {"register", "unique"}:
            value = self._read_storage(varnode.space, varnode.offset, varnode.size)
        else:
            value = self.read_memory(varnode.space, varnode.offset, varnode.size)
        return value & _mask(varnode.size * 8)

    def write(self, varnode: Varnode, value: int) -> None:
        """Write a width-masked value to a register, unique, or memory varnode."""
        if varnode.is_constant:
            raise ValueError("P-Code constants cannot be written")
        value &= _mask(varnode.size * 8)
        if varnode.space in {"register", "unique"}:
            self._write_storage(varnode.space, varnode.offset, varnode.size, value)
        else:
            self.write_memory(varnode.space, varnode.offset, varnode.size, value)

    def _read_storage(self, space: str, offset: int, size: int) -> int:
        """Read sparse byte storage in the configured endian order."""
        data = bytes(self._storage.get((space, offset + index), 0) for index in range(size))
        return int.from_bytes(data, "little" if self.little_endian else "big")

    def _write_storage(self, space: str, offset: int, size: int, value: int) -> None:
        """Write sparse byte storage while retaining overlapping aliases."""
        data = value.to_bytes(size, "little" if self.little_endian else "big")
        for index, byte in enumerate(data):
            self._storage[(space, offset + index)] = byte

    def read_memory(self, space: str, address: int, size: int) -> int:
        """Read an arbitrary sparse memory range with deterministic zero fill."""
        if size < 0:
            raise ValueError(f"Memory read size cannot be negative: {size}")
        segment = self._memory.get(space, {})
        data = bytes(segment.get(address + index, 0) for index in range(size))
        return int.from_bytes(data, "little" if self.little_endian else "big")

    def write_memory(self, space: str, address: int, size: int, value: int) -> None:
        """Write an arbitrary sparse memory range, creating only touched bytes."""
        if size < 0:
            raise ValueError(f"Memory write size cannot be negative: {size}")
        segment = self._memory.setdefault(space, {})
        data = (value & _mask(max(1, size * 8))).to_bytes(size, "little" if self.little_endian else "big")
        for index, byte in enumerate(data):
            segment[address + index] = byte

    def snapshot(self) -> StateSnapshot:
        """Capture a detached sparse snapshot for observer comparisons."""
        storage: dict[tuple[str, int, int], int] = {}
        for space, offset, size in self._locations:
            storage[(space, offset, size)] = self._read_storage(space, offset, size)
        labels = {key: self._descriptions[key] for key in storage if key in self._descriptions}
        memory = {space: dict(segment) for space, segment in self._memory.items()}
        return StateSnapshot(storage=storage, labels=labels, memory=memory, pc=self.pc, halted=self.halted)

    def format_storage_key(self, key: tuple[str, int, int]) -> str:
        """Format a snapshot storage key using known register names when possible."""
        space, offset, size = key
        name = self._descriptions.get((space, offset, size))
        return name or f"{space}[0x{offset:x}:{size}B]"

    def storage_value(self, snapshot: StateSnapshot, key: tuple[str, int, int]) -> int:
        """Return a byte value from a snapshot, defaulting unmapped bytes to zero."""
        return snapshot.storage.get(key, 0)

    def set_register(self, offset: int, size: int, value: int, name: str | None = None) -> None:
        """Initialize a register-space varnode used by the extracted program."""
        varnode = Varnode("register", offset, size, name=name)
        self.describe(varnode)
        self.write(varnode, value)

    def set_memory(self, space: str, address: int, data: bytes) -> None:
        """Initialize a raw memory block without imposing a high-level type."""
        segment = self._memory.setdefault(space, {})
        segment.update({address + index: byte for index, byte in enumerate(data)})


class PcodeExecutionError(RuntimeError):
    """Diagnostic failure raised for unsupported or invalid concrete P-Code."""


class PcodeEmulator:
    """Execute P-Code one operation at a time with extensible event hooks."""

    def __init__(
        self,
        state: MachineState | None = None,
        *,
        address_space_ids: Mapping[int, str] | None = None,
        userops: Mapping[str, Callable[["PcodeEmulator", PcodeOp], None]] | None = None,
    ) -> None:
        """Create an emulator around explicit state and optional user-op handlers."""
        self.state = state or MachineState()
        self.address_space_ids = dict(address_space_ids or {})
        self.userops = dict(userops or {})
        self.observers: list[PcodeObserver] = []
        self.steps = 0
        self.current_function = "<anonymous>"
        self._program: list[PcodeOp] = []
        self._targets: dict[int, int] = {}
        self._call_stack: list[int] = []

    def add_observer(self, observer: PcodeObserver) -> None:
        """Subscribe an observer without coupling it to opcode implementations."""
        self.observers.append(observer)

    def execute(self, op: PcodeOp) -> None:
        """Execute exactly one operation at the current program counter."""
        before = self.state.snapshot()
        step = self.steps
        for observer in self.observers:
            callback = getattr(observer, "before_op", None)
            if callback:
                callback(self, op, step, before)
        try:
            self._execute_op(op)
        except Exception as error:
            if isinstance(error, PcodeExecutionError):
                raise
            raise PcodeExecutionError(
                f"P-Code execution failed at function={self.current_function!r}, step={step}, "
                f"pc={self.state.pc}, op={op.format()}: {error}"
            ) from error
        after = self.state.snapshot()
        for observer in self.observers:
            callback = getattr(observer, "after_op", None)
            if callback:
                callback(self, op, step, before, after)
        self.steps += 1

    def execute_program(self, ops: Iterable[PcodeOp], *, function_name: str = "<anonymous>", max_steps: int = 100000) -> MachineState:
        """Execute a flattened P-Code program, following concrete branches and calls."""
        self._program = list(ops)
        if not self._program:
            raise PcodeExecutionError(f"Cannot execute empty P-Code program for {function_name}")
        self.current_function = function_name
        self._targets = self._build_target_map(self._program)
        self.state.pc = 0
        self.state.halted = False
        self.steps = 0
        while not self.state.halted:
            if self.steps >= max_steps:
                raise PcodeExecutionError(
                    f"Step limit {max_steps} exceeded in function={function_name!r}; "
                    f"possible non-terminating control flow at pc={self.state.pc}"
                )
            if not 0 <= self.state.pc < len(self._program):
                raise PcodeExecutionError(
                    f"Program counter {self.state.pc} left program of {len(self._program)} ops "
                    f"in function={function_name!r}"
                )
            op = self._program[self.state.pc]
            self.execute(op)
        return self.state

    def _build_target_map(self, ops: list[PcodeOp]) -> dict[int, int]:
        """Map Ghidra sequence addresses and local indices to flattened op indices."""
        targets: dict[int, int] = {}
        for index, op in enumerate(ops):
            targets.setdefault(index, index)
            if op.address is not None:
                targets.setdefault(op.address, index)
            if op.sequence is not None:
                targets.setdefault(op.sequence, index)
        return targets

    def _read(self, node: Varnode) -> int:
        """Read a varnode and notify register/unique observers."""
        value = self.state.read(node)
        if node.space in {"register", "unique"}:
            for observer in self.observers:
                callback = getattr(observer, "on_register_read", None)
                if callback:
                    callback(node, value)
        return value

    def _write(self, node: Varnode, value: int) -> None:
        """Write a varnode and notify register/unique observers."""
        self.state.describe(node)
        self.state.write(node, value)
        if node.space in {"register", "unique"}:
            for observer in self.observers:
                callback = getattr(observer, "on_register_write", None)
                if callback:
                    callback(node, value & _mask(node.size * 8))

    def _memory_space(self, node: Varnode) -> str:
        """Resolve a LOAD/STORE space-id constant to a sparse state segment."""
        if node.space_id is not None:
            return self.address_space_ids.get(node.space_id, node.name or "ram")
        return self.address_space_ids.get(self._read(node), node.name or "ram")

    def _load(self, space: str, address: int, size: int) -> int:
        """Load a concrete integer and emit the memory-read event."""
        value = self.state.read_memory(space, address, size)
        for observer in self.observers:
            callback = getattr(observer, "on_memory_read", None)
            if callback:
                callback(space, address, size, value)
        return value

    def _store(self, space: str, address: int, size: int, value: int) -> None:
        """Store a concrete integer and emit the memory-write event."""
        self.state.write_memory(space, address, size, value)
        for observer in self.observers:
            callback = getattr(observer, "on_memory_write", None)
            if callback:
                callback(space, address, size, value & _mask(size * 8))

    def _target(self, op: PcodeOp, node: Varnode) -> int:
        """Resolve an internal relative or external absolute branch target.

        Ghidra's ``PcodeFrame`` treats a constant-space branch as a relative
        operation offset within the current P-Code frame.  The adapter flattens
        instruction frames into one list, so the equivalent target is the
        current flattened index plus that relative offset.  Non-constant
        targets are concrete addresses and use the extracted address map.
        """
        if node.is_constant:
            target = self.state.pc + node.offset
            if target in self._targets:
                return self._targets[target]
            raise PcodeExecutionError(
                f"Unresolved relative branch target {node.offset} from pc={self.state.pc} "
                f"in function={self.current_function!r}"
            )
        # A direct Ghidra branch target is a RAM/address-space Varnode.  Its
        # offset is the address literal, not a request to load bytes from RAM.
        raw = node.offset if node.space not in {"register", "unique", "stack"} else self._read(node)
        if raw in self._targets:
            return self._targets[raw]
        raise PcodeExecutionError(
            f"Unresolved branch target 0x{raw:x} at function={self.current_function!r}, "
            f"pc={self.state.pc}; known targets={sorted(self._targets)[:20]}"
        )

    def _binary(self, op: PcodeOp, function: Callable[[int, int, int], int]) -> None:
        """Evaluate a two-input integer operation and write its output width."""
        if op.output is None or len(op.inputs) < 2:
            raise PcodeExecutionError(f"Malformed binary operation: {op.format()}")
        left = self._read(op.inputs[0])
        right = self._read(op.inputs[1])
        self._write(op.output, function(left, right, op.output.size))
        self.state.pc += 1

    def _unary(self, op: PcodeOp, function: Callable[[int, int], int]) -> None:
        """Evaluate a one-input operation and write its output width."""
        if op.output is None or not op.inputs:
            raise PcodeExecutionError(f"Malformed unary operation: {op.format()}")
        self._write(op.output, function(self._read(op.inputs[0]), op.output.size))
        self.state.pc += 1

    def _execute_op(self, op: PcodeOp) -> None:
        """Dispatch one operation, preserving P-Code width and control semantics."""
        name = op.mnemonic.upper()
        if name == "COPY":
            self._unary(op, lambda value, _size: value)
        elif name == "LOAD":
            if op.output is None or len(op.inputs) < 2:
                raise PcodeExecutionError(f"Malformed LOAD: {op.format()}")
            space = self._memory_space(op.inputs[0])
            address = self._read(op.inputs[1])
            self._write(op.output, self._load(space, address, op.output.size))
            self.state.pc += 1
        elif name == "STORE":
            if len(op.inputs) < 3:
                raise PcodeExecutionError(f"Malformed STORE: {op.format()}")
            space = self._memory_space(op.inputs[0])
            address = self._read(op.inputs[1])
            value_node = op.inputs[2]
            self._store(space, address, value_node.size, self._read(value_node))
            self.state.pc += 1
        elif name == "INT_ADD":
            self._binary(op, lambda a, b, size: a + b)
        elif name == "INT_SUB":
            self._binary(op, lambda a, b, size: a - b)
        elif name == "INT_MULT":
            self._binary(op, lambda a, b, size: a * b)
        elif name == "INT_XOR":
            self._binary(op, lambda a, b, size: a ^ b)
        elif name == "INT_AND":
            self._binary(op, lambda a, b, size: a & b)
        elif name == "INT_OR":
            self._binary(op, lambda a, b, size: a | b)
        elif name == "INT_LEFT":
            input_bits = op.inputs[0].size * 8
            self._binary(op, lambda a, b, size: a << b if b < input_bits else 0)
        elif name == "INT_RIGHT":
            input_bits = op.inputs[0].size * 8
            self._binary(op, lambda a, b, size: a >> b if b < input_bits else 0)
        elif name == "INT_SRIGHT":
            input_size = op.inputs[0].size
            input_bits = input_size * 8
            self._binary(op, lambda a, b, size: (_signed(a, input_size) >> b) if b < input_bits else (-1 if _signed(a, input_size) < 0 else 0))
        elif name in {"INT_EQUAL", "INT_NOTEQUAL", "INT_LESS", "INT_LESSEQUAL", "INT_SLESS", "INT_SLESSEQUAL"}:
            def compare(a: int, b: int, size: int) -> int:
                if name == "INT_EQUAL":
                    return int(a == b)
                if name == "INT_NOTEQUAL":
                    return int(a != b)
                if name == "INT_LESS":
                    return int(a < b)
                if name == "INT_LESSEQUAL":
                    return int(a <= b)
                if name == "INT_SLESS":
                    return int(_signed(a, op.inputs[0].size) < _signed(b, op.inputs[1].size))
                return int(_signed(a, op.inputs[0].size) <= _signed(b, op.inputs[1].size))
            self._binary(op, compare)
        elif name == "INT_ZEXT":
            self._unary(op, lambda value, size: value)
        elif name == "INT_SEXT":
            if op.output is None or not op.inputs:
                raise PcodeExecutionError(f"Malformed INT_SEXT: {op.format()}")
            self._write(op.output, _signed(self._read(op.inputs[0]), op.inputs[0].size))
            self.state.pc += 1
        elif name == "INT_2COMP":
            self._unary(op, lambda value, size: -value)
        elif name == "INT_NEGATE":
            self._unary(op, lambda value, size: ~value)
        elif name == "INT_CARRY":
            input_bits = op.inputs[0].size * 8
            self._binary(op, lambda a, b, size: int((a + b) > _mask(input_bits)))
        elif name == "INT_SCARRY":
            def signed_carry(a: int, b: int, size: int) -> int:
                input_size = op.inputs[0].size
                result = _signed(a, input_size) + _signed(b, input_size)
                return int(result < -(1 << (input_size * 8 - 1)) or result > (1 << (input_size * 8 - 1)) - 1)
            self._binary(op, signed_carry)
        elif name == "INT_SBORROW":
            def signed_borrow(a: int, b: int, size: int) -> int:
                input_size = op.inputs[0].size
                result = _signed(a, input_size) - _signed(b, input_size)
                return int(result < -(1 << (input_size * 8 - 1)) or result > (1 << (input_size * 8 - 1)) - 1)
            self._binary(op, signed_borrow)
        elif name in {"INT_DIV", "INT_REM", "INT_SDIV", "INT_SREM"}:
            if op.output is None or len(op.inputs) < 2:
                raise PcodeExecutionError(f"Malformed division operation: {op.format()}")
            a = self._read(op.inputs[0])
            b = self._read(op.inputs[1])
            if b == 0 and name == "INT_DIV":
                self._write(op.output, 0)
                self.state.pc += 1
                return
            if b == 0:
                raise PcodeExecutionError(f"Division by zero in {op.format()}")
            if name in {"INT_SDIV", "INT_SREM"}:
                dividend = _signed(a, op.inputs[0].size)
                divisor = _signed(b, op.inputs[1].size)
                quotient = abs(dividend) // abs(divisor)
                quotient = -quotient if (dividend < 0) != (divisor < 0) else quotient
                result = quotient if name == "INT_SDIV" else dividend - quotient * divisor
            else:
                result = a // b if name == "INT_DIV" else a % b
            self._write(op.output, result)
            self.state.pc += 1
        elif name in {"BOOL_NEGATE", "BOOL_XOR", "BOOL_AND", "BOOL_OR"}:
            if name == "BOOL_NEGATE":
                self._unary(op, lambda value, size: value ^ 1)
            else:
                self._binary(op, {
                    # Ghidra's boolean behaviors are bitwise operations; the
                    # caller is responsible for the output varnode width.
                    "BOOL_XOR": lambda a, b, size: a ^ b,
                    "BOOL_AND": lambda a, b, size: a & b,
                    "BOOL_OR": lambda a, b, size: a | b,
                }[name])
        elif name == "PIECE":
            if op.output is None or len(op.inputs) < 2:
                raise PcodeExecutionError(f"Malformed PIECE: {op.format()}")
            high, low = (self._read(node) for node in op.inputs[:2])
            self._write(op.output, (high << (op.inputs[1].size * 8)) | low)
            self.state.pc += 1
        elif name == "SUBPIECE":
            if op.output is None or len(op.inputs) < 2:
                raise PcodeExecutionError(f"Malformed SUBPIECE: {op.format()}")
            source = self._read(op.inputs[0])
            byte_offset = self._read(op.inputs[1])
            self._write(op.output, source >> (byte_offset * 8))
            self.state.pc += 1
        elif name == "POPCOUNT":
            if op.output is None or not op.inputs:
                raise PcodeExecutionError(f"Malformed POPCOUNT: {op.format()}")
            value = self._read(op.inputs[0])
            self._write(op.output, value.bit_count())
            self.state.pc += 1
        elif name == "LZCOUNT":
            if op.output is None or not op.inputs:
                raise PcodeExecutionError(f"Malformed LZCOUNT: {op.format()}")
            value = self._read(op.inputs[0]) & _mask(op.inputs[0].size * 8)
            width = op.inputs[0].size * 8
            self._write(op.output, width if value == 0 else width - value.bit_length())
            self.state.pc += 1
        elif name in {"BRANCH", "CBRANCH"}:
            if not op.inputs:
                raise PcodeExecutionError(f"Malformed {name}: {op.format()}")
            target = self._target(op, op.inputs[0])
            taken = name == "BRANCH" or bool(self._read(op.inputs[1]))
            for observer in self.observers:
                callback = getattr(observer, "on_branch", None)
                if callback:
                    callback(op, target, taken)
            if taken:
                self.state.pc = target
            else:
                self.state.pc += 1
        elif name == "CALL":
            if not op.inputs:
                raise PcodeExecutionError(f"Malformed CALL: {op.format()}")
            target = self._target(op, op.inputs[0])
            self._call_stack.append(self.state.pc + 1)
            for observer in self.observers:
                callback = getattr(observer, "on_call", None)
                if callback:
                    callback(op, target)
            self.state.pc = target
        elif name == "RETURN":
            for observer in self.observers:
                callback = getattr(observer, "on_return", None)
                if callback:
                    callback(op)
            if self._call_stack:
                self.state.pc = self._call_stack.pop()
            else:
                self.state.halted = True
        elif name == "CALLOTHER":
            userop_name = op.inputs[0].name if op.inputs and op.inputs[0].name else "<unknown>"
            handler = self.userops.get(userop_name)
            if handler is None:
                raise PcodeExecutionError(
                    f"Unsupported CALLOTHER userop={userop_name!r}, function={self.current_function!r}, "
                    f"step={self.steps}, pc={self.state.pc}, op={op.format()}"
                )
            handler(self, op)
            self.state.pc += 1
        else:
            raise PcodeExecutionError(
                f"Unsupported P-Code opcode={name!r}, function={self.current_function!r}, "
                f"step={self.steps}, pc={self.state.pc}, op={op.format()}"
            )
