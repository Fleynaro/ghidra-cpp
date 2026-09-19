"""Extract real Ghidra P-Code, execute it in the custom core, and write reports."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from src.pcode_emulator import MachineState, PcodeEmulator, PcodeOp, Varnode
from src.trace import MarkdownTraceRecorder, MemoryRegion


FUNCTIONS = {
    "add": {"args": (10, 32), "expected": 42, "regions": []},
    "sum_array": {"args": (0x10000000, 4), "memory": [(0x10000000, [1, 2, 3, 4])], "expected": 10, "regions": [(0x10000000, 16, "int[4] values", 4)]},
    "add_to_list": {"args": (0x10000000, 4, 5), "memory": [(0x10000000, [10, 20, 30, 40])], "expected": [15, 25, 35, 45], "regions": [(0x10000000, 16, "int[4] values", 4)]},
    "multiply": {"args": (7, 6), "expected": 42, "regions": []},
    "absolute_difference": {"args": (9, 23), "expected": 14, "regions": []},
    "count_even": {"args": (0x10000000, 4), "memory": [(0x10000000, [1, 2, 3, 4])], "expected": 2, "regions": [(0x10000000, 16, "int[4] values", 4)]},
    "clamp_value": {"args": (99, 0, 50), "expected": 50, "regions": []},
    "dot_product": {"args": (0x10000000, 0x10000100, 3), "memory": [(0x10000000, [1, 2, 3]), (0x10000100, [4, 5, 6])], "expected": 32, "regions": [(0x10000000, 12, "int[3] left", 4), (0x10000100, 12, "int[3] right", 4)]},
    "reverse_in_place": {"args": (0x10000000, 4), "memory": [(0x10000000, [10, 20, 30, 40])], "expected": [40, 30, 20, 10], "regions": [(0x10000000, 16, "int[4] values", 4)]},
    "choose_value": {"args": (1, 11, 22), "expected": 11, "regions": []},
}


def java_long(value: object) -> int:
    """Convert a Java long/int-like value returned through PyGhidra to Python int."""
    return int(value)


def register_descriptor(language: object, name: str) -> tuple[int, int]:
    """Return the register-space offset and byte width for a named register."""
    register = language.getRegister(name)
    if register is None:
        raise RuntimeError(f"Ghidra language does not define register {name}")
    address = register.getAddress()
    try:
        size = int(register.getMinimumByteSize())
    except AttributeError:
        size = int(register.getBitLength()) // 8
    return java_long(address.getOffset()), size


def convert_varnode(varnode: object, language: object, spaces: dict[int, str]) -> Varnode:
    """Translate one Ghidra Varnode into the core's serializable descriptor."""
    address = varnode.getAddress()
    space = address.getAddressSpace()
    space_name = str(space.getName()).lower()
    offset = java_long(varnode.getOffset())
    size = int(varnode.getSize())
    if varnode.isConstant():
        return Varnode("constant", offset, size, name=spaces.get(offset), space_id=offset if size <= 4 else None)
    name = None
    if space_name == "register":
        register = language.getRegister(address, size)
        if register is not None:
            name = str(register.getName())
    elif space_name == "unique":
        name = f"unique_0x{offset:x}"
    return Varnode(space_name, offset, size, name=name, space_id=int(space.getSpaceID()))


def operation_mnemonic(op: object) -> str:
    """Obtain a stable mnemonic across the Ghidra Java/Python API variants."""
    try:
        return str(op.getMnemonic())
    except AttributeError:
        from ghidra.program.model.pcode import PcodeOp as JavaPcodeOp

        return str(JavaPcodeOp.getMnemonic(op.getOpcode()))


def extract_ops(program: object, function: object) -> tuple[list[PcodeOp], dict[int, str]]:
    """Flatten instruction P-Code while retaining machine instruction addresses."""
    language = program.getLanguage()
    spaces: dict[int, str] = {}
    address_space = language.getAddressFactory().getAddressSpace("ram")
    if address_space is not None:
        spaces[int(address_space.getSpaceID())] = "ram"
    ops: list[PcodeOp] = []
    for instruction in program.getListing().getInstructions(function.getBody(), True):
        instruction_address = java_long(instruction.getAddress().getOffset())
        for raw_op in instruction.getPcode(False):
            inputs = tuple(convert_varnode(node, language, spaces) for node in raw_op.getInputs())
            output_node = raw_op.getOutput()
            output = convert_varnode(output_node, language, spaces) if output_node is not None else None
            sequence = raw_op.getSeqnum()
            sequence_time = int(sequence.getTime()) if sequence is not None else None
            ops.append(PcodeOp(operation_mnemonic(raw_op), inputs, output, instruction_address, sequence_time))
    return ops, spaces


def find_function(program: object, name: str) -> object:
    """Find an exported corpus function by exact symbol name."""
    for function in program.getFunctionManager().getFunctions(True):
        if not function.isExternal() and str(function.getName()) == name:
            return function
    raise RuntimeError(f"Function {name!r} was not discovered by Ghidra analysis")


def initialize_state(language: object, case: dict[str, object]) -> MachineState:
    """Seed Windows x64 argument registers, stack return storage, and arrays."""
    state = MachineState(little_endian=not bool(language.isBigEndian()))
    register_names = ("RCX", "RDX", "R8", "R9")
    for name, value in zip(register_names, case["args"]):
        offset, size = register_descriptor(language, name)
        state.set_register(offset, size, int(value), name)
    rsp_offset, rsp_size = register_descriptor(language, "RSP")
    stack_pointer = 0x200000
    state.set_register(rsp_offset, rsp_size, stack_pointer, "RSP")
    state.set_memory("ram", stack_pointer, (0xDEADBEEF).to_bytes(8, "little"))
    for address, values in case.get("memory", []):
        data = b"".join(int(value).to_bytes(4, "little", signed=True) for value in values)
        state.set_memory("ram", int(address), data)
    return state


def parse_normal_results(executable: Path) -> dict[str, object]:
    """Run the ordinary C++ harness and parse its deterministic result lines."""
    completed = subprocess.run([str(executable)], check=True, capture_output=True, text=True)
    results: dict[str, object] = {}
    for line in completed.stdout.splitlines():
        name, raw = line.split("=", 1)
        if "," in raw:
            results[name] = [int(item) for item in raw.split(",")]
        else:
            results[name] = int(raw)
    return results


def result_from_state(language: object, state: MachineState, case: dict[str, object], name: str) -> object:
    """Read the ABI result register or the configured mutated memory block."""
    if isinstance(case["expected"], list):
        address = int(case["regions"][0][0])
        count = len(case["expected"])
        raw = state.read_memory("ram", address, count * 4)
        return [(raw >> (index * 32)) & 0xFFFFFFFF for index in range(count)]
    offset, size = register_descriptor(language, "RAX")
    value = state.read(Varnode("register", offset, size))
    if value & (1 << (size * 8 - 1)):
        value -= 1 << (size * 8)
    return value


def validation_text(expected: object, normal: object, emulated: object) -> str:
    """Render a human-readable three-way validation result."""
    status = "PASS" if expected == normal == emulated else "FAIL"
    return "\n".join([
        "**Expected result:**",
        f"`{expected}`",
        "",
        "**Normal C++ result:**",
        f"`{normal}`",
        "",
        "**Emulated result:**",
        f"`{emulated}`",
        "",
        f"**Status:** `{status}`",
    ])


def run(executable: Path, output: Path) -> int:
    """Run the complete ten-function extraction, emulation, and report pipeline."""
    import pyghidra

    pyghidra.start()
    normal_results = parse_normal_results(executable)
    from ghidra.app.util.opinion import PeLoader

    output.mkdir(parents=True, exist_ok=True)
    failures = 0
    with pyghidra.open_program(executable, analyze=True, loader=PeLoader) as flat_api:
        program = flat_api.getCurrentProgram()
        if program is None:
            raise RuntimeError("PyGhidra did not produce a Program")
        language = program.getLanguage()
        for name, case in FUNCTIONS.items():
            function = find_function(program, name)
            ops, spaces = extract_ops(program, function)
            state = initialize_state(language, case)
            recorder = MarkdownTraceRecorder(
                function_name=name,
                arguments=str(case["args"]),
                regions=[MemoryRegion("ram", int(address), int(size), label, int(element_size), True) for address, size, label, element_size in case.get("regions", [])],
            )
            emulator = PcodeEmulator(state, address_space_ids=spaces)
            emulator.add_observer(recorder)
            emulator.execute_program(ops, function_name=name)
            emulated = result_from_state(language, state, case, name)
            expected = case["expected"]
            normal = normal_results.get(name)
            if normal is None:
                raise RuntimeError(f"Normal C++ harness did not emit a result for {name}")
            if expected != normal or expected != emulated:
                failures += 1
            report = recorder.render(validation=validation_text(expected, normal, emulated))
            (output / f"{name}.md").write_text(report, encoding="utf-8")
            print(f"{name}: {'PASS' if expected == normal == emulated else 'FAIL'} ({len(ops)} P-Code ops)")
    return failures


def main() -> int:
    """Parse command-line paths and return a process status for CI."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", type=Path, default=ROOT / "corpus.exe")
    parser.add_argument("--output", type=Path, default=ROOT / "output")
    args = parser.parse_args()
    if not args.executable.is_file():
        raise SystemExit(f"Executable not found: {args.executable}; run build.bat first")
    return run(args.executable.resolve(), args.output.resolve())


if __name__ == "__main__":
    raise SystemExit(main())
