# Call-Fixup Installer Behavioral Fixture

> Generated automatically with PyGhidra.

## Input

- **File:** `test_call_fixup_installer.exe`
- **File size:** `2560` bytes

## Analysis Configuration

- **Enabled boolean analyzers:** `Call-Fixup Installer`
- **Compiler-spec target:** `__security_check_cookie`
- **Expected payload:** `security_check_cookie`

## Before target analysis

| Name | Call fixup | No return |
| --- | --- | --- |
| `__security_check_cookie` | `None` | `False` |
| `call_fixup_installer_entry` | `None` | `False` |

## After target analysis

| Name | Call fixup | No return |
| --- | --- | --- |
| `__security_check_cookie` | `security_check_cookie` | `False` |
| `call_fixup_installer_entry` | `None` | `False` |

## Delta

**Added rows**

- None

**Removed rows**

- None

**Changed rows**

- Before `__security_check_cookie` | `None` | `False`; after `__security_check_cookie` | `security_check_cookie` | `False`


## Function State

| Name | Call fixup | No return | Changed |
| --- | --- | --- | --- |
| `__security_check_cookie` | `security_check_cookie` | `False` | `true` |
| `call_fixup_installer_entry` | `None` | `False` | `false` |

- **Functions with installed call fixups:** `1`.
