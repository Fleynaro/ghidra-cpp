# Call-Fixup Installer Behavioral Fixture

> Generated automatically with PyGhidra.

## Input

- **File:** `test_call_fixup_installer.exe`
- **File size:** `2560` bytes

## Analysis Configuration

- **Enabled boolean analyzers:** `Call-Fixup Installer`
- **Compiler-spec target:** `__security_check_cookie`
- **Expected payload:** `security_check_cookie`

## Function State

| Name | Call fixup | No return | Changed |
| --- | --- | --- | --- |
| `__security_check_cookie` | `security_check_cookie` | `False` | `true` |
| `call_fixup_installer_entry` | `None` | `False` | `false` |

- **Functions with installed call fixups:** `1`.
