"""Custom P-Code emulation and trace tooling for function-recognition research."""

from .pcode_emulator import (
    MachineState,
    PcodeEmulator,
    PcodeExecutionError,
    PcodeOp,
    PcodeObserver,
    Varnode,
)
from .trace import MarkdownTraceRecorder, MemoryRegion

__all__ = [
    "MachineState",
    "MarkdownTraceRecorder",
    "MemoryRegion",
    "PcodeEmulator",
    "PcodeExecutionError",
    "PcodeOp",
    "PcodeObserver",
    "Varnode",
    "MemoryRegion",
]
