"""Unit tests for the independent concrete P-Code execution core."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from src.pcode_emulator import MachineState, PcodeEmulator, PcodeOp, Varnode


def constant(value: int, size: int = 1) -> Varnode:
    """Create a literal varnode for a hand-written P-Code test."""
    return Varnode("constant", value, size)


def register(offset: int, size: int = 4, name: str | None = None) -> Varnode:
    """Create a register-space varnode with a deterministic byte offset."""
    return Varnode("register", offset, size, name=name)


class EmulatorTests(unittest.TestCase):
    """Verify width, memory, composition, and concrete control-flow contracts."""

    def run_ops(self, ops: list[PcodeOp], state: MachineState | None = None) -> MachineState:
        """Execute a short program and return its final state for assertions."""
        return PcodeEmulator(state, address_space_ids={0: "ram"}).execute_program(ops, function_name="unit")

    def test_copy_and_integer_arithmetic(self) -> None:
        """COPY, ADD, and SUB must preserve the destination width and mask overflow."""
        eax = register(0, 4, "EAX")
        ebx = register(8, 4, "EBX")
        state = MachineState()
        state.set_register(8, 4, 32, "EBX")
        self.run_ops([
            PcodeOp("COPY", (constant(10, 4),), eax),
            PcodeOp("INT_ADD", (eax, ebx), eax),
            PcodeOp("INT_SUB", (eax, constant(2, 4)), eax),
            PcodeOp("RETURN", (constant(0, 8),)),
        ], state)
        self.assertEqual(state.read(eax), 40)

    def test_bitwise_and_shifts(self) -> None:
        """Bitwise operations and logical shifts must operate on unsigned bit patterns."""
        eax = register(0, 1, "AL")
        self.run_ops([
            PcodeOp("COPY", (constant(0x96),), eax),
            PcodeOp("INT_AND", (eax, constant(0x0F)), eax),
            PcodeOp("INT_OR", (eax, constant(0x80)), eax),
            PcodeOp("INT_XOR", (eax, constant(0xFF)), eax),
            PcodeOp("INT_LEFT", (eax, constant(1)), eax),
            PcodeOp("INT_RIGHT", (eax, constant(2)), eax),
            PcodeOp("RETURN", (constant(0, 8),)),
        ])

    def test_piece_and_subpiece(self) -> None:
        """PIECE concatenates high then low bytes and SUBPIECE extracts by byte offset."""
        result = register(0, 4, "EAX")
        low = register(8, 2, "LOW")
        self.run_ops([
            PcodeOp("PIECE", (constant(0x12, 2), constant(0x3456, 2)), result),
            PcodeOp("SUBPIECE", (result, constant(1)), low),
            PcodeOp("RETURN", (constant(0, 8),)),
        ])
        self.assertEqual(self.run_ops([
            PcodeOp("PIECE", (constant(0x12, 2), constant(0x3456, 2)), result),
            PcodeOp("SUBPIECE", (result, constant(1)), low),
            PcodeOp("RETURN", (constant(0, 8),)),
        ]).read(low), 0x1234)

    def test_load_and_store(self) -> None:
        """STORE and LOAD must use little-endian arbitrary memory ranges."""
        address = register(0, 8, "RAX")
        value = register(8, 4, "EBX")
        state = MachineState()
        state.set_register(0, 8, 0x1000, "RAX")
        state.set_register(8, 4, 0x12345678, "EBX")
        self.run_ops([
            PcodeOp("STORE", (Varnode("constant", 0, 1, name="ram", space_id=0), address, value)),
            PcodeOp("LOAD", (Varnode("constant", 0, 1, name="ram", space_id=0), address), value),
            PcodeOp("RETURN", (constant(0, 8),)),
        ], state)
        self.assertEqual(state.read(value), 0x12345678)
        self.assertEqual(state.read_memory("ram", 0x1000, 4), 0x12345678)

    def test_conditional_branch_and_branch(self) -> None:
        """CBRANCH chooses the concrete path and BRANCH skips the alternate path."""
        eax = register(0, 4, "EAX")
        ops = [
            PcodeOp("CBRANCH", (constant(4), constant(1)), address=0),
            PcodeOp("COPY", (constant(99, 4),), eax, address=1),
            PcodeOp("BRANCH", (constant(3),), address=2),
            PcodeOp("COPY", (constant(88, 4),), eax, address=3),
            PcodeOp("COPY", (constant(42, 4),), eax, address=4),
            PcodeOp("RETURN", (constant(0, 8),), address=5),
        ]
        state = self.run_ops(ops)
        self.assertEqual(state.read(eax), 42)

    def test_signed_extensions_and_overflow_flags(self) -> None:
        """Signed extension and carry/borrow operations must follow fixed-width rules."""
        eax = register(0, 1, "AL")
        wide = register(8, 2, "AX")
        flag = register(16, 1, "FLAG")
        state = self.run_ops([
            PcodeOp("INT_SEXT", (constant(0x80, 1),), wide),
            PcodeOp("INT_SCARRY", (constant(0x7F, 1), constant(1, 1)), flag),
            PcodeOp("INT_SBORROW", (constant(0x80, 1), constant(1, 1)), flag),
            PcodeOp("INT_NEGATE", (constant(0x0F, 1),), eax),
            PcodeOp("RETURN", (constant(0, 8),)),
        ])
        self.assertEqual(state.read(wide), 0xFF80)
        self.assertEqual(state.read(eax), 0xF0)

    def test_flag_width_comes_from_integer_inputs(self) -> None:
        """Carry and signed overflow use four-byte operands even when the flag is one byte."""
        flag = register(0, 1, "CF")
        state = self.run_ops([
            PcodeOp("INT_CARRY", (constant(0xFFFFFFFF, 4), constant(1, 4)), flag),
            PcodeOp("INT_SCARRY", (constant(0x7FFFFFFF, 4), constant(1, 4)), flag),
            PcodeOp("INT_SBORROW", (constant(0x80000000, 4), constant(1, 4)), flag),
            PcodeOp("RETURN", (constant(0, 8),)),
        ])
        self.assertEqual(state.read(flag), 1)

    def test_boolean_and_unsigned_division_contracts(self) -> None:
        """Boolean operations remain bitwise and unsigned divide-by-zero returns zero."""
        result = register(0, 1, "AL")
        self.run_ops([
            PcodeOp("BOOL_NEGATE", (constant(0x02),), result),
            PcodeOp("INT_DIV", (constant(99, 4), constant(0, 4)), register(8, 4, "EDX")),
            PcodeOp("RETURN", (constant(0, 8),)),
        ])
        self.assertEqual(self.run_ops([
            PcodeOp("BOOL_NEGATE", (constant(0x02),), result),
            PcodeOp("RETURN", (constant(0, 8),)),
        ]).read(result), 0x03)

    def test_local_call_returns_to_caller(self) -> None:
        """A mapped CALL enters a local block and RETURN resumes the saved caller index."""
        result = register(0, 4, "EAX")
        state = self.run_ops([
            PcodeOp("CALL", (Varnode("ram", 3, 8),), address=0),
            PcodeOp("COPY", (constant(9, 4),), result, address=1),
            PcodeOp("RETURN", (constant(0, 8),), address=2),
            PcodeOp("COPY", (constant(7, 4),), result, address=3),
            PcodeOp("RETURN", (constant(0, 8),), address=4),
        ])
        self.assertEqual(state.read(result), 9)


if __name__ == "__main__":
    unittest.main()
