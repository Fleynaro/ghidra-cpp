"""Observer-based Markdown execution trace generation."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from .pcode_emulator import MachineState, PcodeOp, StateSnapshot, Varnode


@dataclass(frozen=True)
class MemoryRegion:
    """Describe a known input block for readable, explicitly labelled rendering."""

    space: str
    address: int
    size: int
    label: str
    element_size: int = 1
    signed: bool = False


@dataclass
class _Step:
    """Accumulate one operation's events until the emulator publishes its result."""

    number: int
    op: PcodeOp
    before: StateSnapshot
    reads: list[str]
    writes: list[str]
    branch: str | None = None


class MarkdownTraceRecorder:
    """Record emulator observer events and render readable state diffs."""

    def __init__(self, *, function_name: str, arguments: str, regions: list[MemoryRegion] | None = None) -> None:
        """Create an independent recorder with optional memory-region hints."""
        self.function_name = function_name
        self.arguments = arguments
        self.regions = regions or []
        self.steps: list[tuple[_Step, StateSnapshot]] = []
        self.initial: StateSnapshot | None = None
        self.final: StateSnapshot | None = None
        self._active: _Step | None = None

    def before_op(self, emulator: Any, op: PcodeOp, step: int, state: StateSnapshot) -> None:
        """Capture the immutable before-state at exactly one P-Code boundary."""
        if self.initial is None:
            self.initial = state
        self._active = _Step(step, op, state, [], [])

    def after_op(self, emulator: Any, op: PcodeOp, step: int, before: StateSnapshot, after: StateSnapshot) -> None:
        """Store one complete operation record, including its state transition."""
        if self._active is None:
            self._active = _Step(step, op, before, [], [])
        self.steps.append((self._active, after))
        self.final = after
        self._active = None

    def on_memory_read(self, space: str, address: int, size: int, value: int) -> None:
        """Record concrete reads inside the currently executing operation."""
        if self._active is not None:
            self._active.reads.append(f"{space}:0x{address:x}/{size}B = 0x{value:x}")

    def on_memory_write(self, space: str, address: int, size: int, value: int) -> None:
        """Record concrete writes inside the currently executing operation."""
        if self._active is not None:
            self._active.writes.append(f"{space}:0x{address:x}/{size}B = 0x{value:x}")

    def on_branch(self, op: PcodeOp, target: int, taken: bool) -> None:
        """Record whether a concrete branch target was taken or skipped."""
        if self._active is not None:
            self._active.branch = f"{'branch taken' if taken else 'fall-through'} (target {target})"

    def on_register_read(self, varnode: Varnode, value: int) -> None:
        """Keep the hook available for future provenance views."""

    def on_register_write(self, varnode: Varnode, value: int) -> None:
        """Keep the hook available for future provenance views."""

    def on_call(self, op: PcodeOp, target: int) -> None:
        """Record call control-flow metadata without changing execution."""
        if self._active is not None:
            self._active.branch = f"call target {target}"

    def on_return(self, op: PcodeOp) -> None:
        """Record return control-flow metadata without changing execution."""
        if self._active is not None:
            self._active.branch = "return"

    def _changed_storage(self, before: StateSnapshot, after: StateSnapshot) -> list[tuple[tuple[str, int, int], int | None, int | None]]:
        """Return changed sparse storage byte locations with old and new values."""
        keys = set(before.storage) | set(after.storage)
        return sorted((key, before.storage.get(key), after.storage.get(key)) for key in keys if before.storage.get(key) != after.storage.get(key))

    def _changed_memory(self, before: StateSnapshot, after: StateSnapshot) -> list[tuple[str, int, int | None, int | None]]:
        """Return changed sparse memory bytes with old and new values."""
        changes: list[tuple[str, int, int | None, int | None]] = []
        spaces = set(before.memory) | set(after.memory)
        for space in spaces:
            old = before.memory.get(space, {})
            new = after.memory.get(space, {})
            for address in sorted(set(old) | set(new)):
                if old.get(address) != new.get(address):
                    changes.append((space, address, old.get(address), new.get(address)))
        return changes

    def _render_value(self, value: int | None) -> str:
        """Format an optional byte value compactly."""
        return "<unmapped>" if value is None else f"0x{value:02x} ({value})"

    def _render_region(self, snapshot: StateSnapshot, region: MemoryRegion) -> str:
        """Render a configured contiguous region as bytes or signed integer elements."""
        memory = snapshot.memory.get(region.space, {})
        raw = bytes(memory.get(region.address + index, 0) for index in range(region.size))
        if region.element_size == 1:
            values = " ".join(f"{byte:02x}" for byte in raw)
        else:
            values_list = []
            for index in range(0, region.size, region.element_size):
                chunk = raw[index : index + region.element_size]
                value = int.from_bytes(chunk, "little")
                if region.signed and value & (1 << (region.element_size * 8 - 1)):
                    value -= 1 << (region.element_size * 8)
                values_list.append(str(value))
            values = ", ".join(values_list)
        return f"- `{region.label}` @ `0x{region.address:x}` ({region.size} bytes): `{values}`"

    def render(self, *, validation: str = "Not supplied") -> str:
        """Render all captured steps, diffs, control flow, and validation text."""
        initial = self.initial
        final = self.final
        lines = [
            f"# P-Code Execution Trace: `{self.function_name}`",
            "",
            f"**Arguments:** `{self.arguments}`",
            "",
            "## Initial State",
            "",
            "Sparse register/temporary state is shown only when a byte location is changed.",
            "",
        ]
        if initial is not None:
            lines.append(f"- Program counter: `{initial.pc}`")
            for key, value in sorted(initial.storage.items()):
                label = initial.labels.get(key, f"{key[0]}[0x{key[1]:x}:{key[2]}B]")
                lines.append(f"- `{label}` = `0x{value:x}` ({value})")
            for region in self.regions:
                lines.append(self._render_region(initial, region))
        lines.extend(["", "## Execution Steps", ""])
        for record, after in self.steps:
            lines.extend([f"### Step {record.number}", "", f"**P-Code:** `{record.op.format()}`", ""])
            if record.reads:
                lines.append("**Memory reads:** " + "; ".join(f"`{item}`" for item in record.reads))
            if record.writes:
                lines.append("**Memory writes:** " + "; ".join(f"`{item}`" for item in record.writes))
            if record.branch:
                lines.append(f"**Control flow:** {record.branch}")
            lines.append(f"**Program counter:** `{record.before.pc}` -> `{after.pc}`")
            changed_storage = self._changed_storage(record.before, after)
            lines.extend(["", "**State before/after:**"])
            if changed_storage:
                for key, old, new in changed_storage:
                    label = after.labels.get(key, record.before.labels.get(key, f"{key[0]}[0x{key[1]:x}:{key[2]}B]"))
                    lines.append(f"- `{label}`: `{self._render_value(old)}` -> `{self._render_value(new)}`")
            else:
                lines.append("- No register or unique-byte changes.")
            changed_memory = self._changed_memory(record.before, after)
            if changed_memory:
                lines.append("")
                lines.append("**Memory diff:**")
                for space, address, old, new in changed_memory:
                    lines.append(f"- `{space}:0x{address:x}`: `{self._render_value(old)}` -> `{self._render_value(new)}`")
                for region in self.regions:
                    if any(space == region.space and region.address <= address < region.address + region.size for space, address, _, _ in changed_memory):
                        lines.append(self._render_region(after, region))
            lines.append("")
        lines.extend(["## Final State", "", f"- Program counter: `{final.pc if final else 'unknown'}`", f"- Halted: `{final.halted if final else 'unknown'}`", ""])
        for region in self.regions:
            if final is not None:
                lines.append(self._render_region(final, region))
        lines.extend(["", "## Validation", "", validation, ""])
        return "\n".join(lines)
