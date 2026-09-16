// MSVC x64 integration fixture for Ghidra's "Call-Fixup Installer" analyzer.
//
// Original behavior and traceability:
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/CallFixupAnalyzer.java
//   (added(), getTargetFixupMap(), and getCallFixupNameForFunction()) maps compiler-spec
//   call-fixup target names to payloads, installs a missing fixup, and repairs callers of
//   non-fall-through payloads.
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/CallFixupChangeAnalyzer.java
//   supplies the user-visible `Call-Fixup Installer` name used by this harness.
// * x86/data/languages/x86-64-win.cspec declares `__security_check_cookie` as a target of
//   the `security_check_cookie` payload. The exact exported symbol below is therefore the
//   construct that lets the authoritative analyzer discover the fixup without CRT files.
// * run_ghidra.py extracts installed call-fixup names and caller flow state to markdown.

extern "C" volatile unsigned __int64 call_fixup_sink = 0;

// This symbol deliberately matches the x86-64 Windows compiler-spec target.
extern "C" __declspec(noinline) void
__security_check_cookie(unsigned __int64 cookie) { // NOLINT(bugprone-reserved-identifier)
    call_fixup_sink ^= cookie;
}

// The call is retained and gives the installer a real caller whose flow can be observed.
extern "C" __declspec(noinline) void call_fixup_installer_entry() {
    __security_check_cookie(0xC0FFEEU);
    call_fixup_sink += 1U;
}
