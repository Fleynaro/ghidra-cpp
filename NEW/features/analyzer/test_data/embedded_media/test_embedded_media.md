# Embedded Media Behavioral Fixture

> Generated from the saved MSVC x64 PE program with PyGhidra.

## Input

- **File:** `test_embedded_media.exe`
- **File size:** `3584` bytes

## Analysis Configuration

| Enabled boolean analyzer |
| --- |
| `Embedded Media` |

## Successful Media Data

| Offset | Data type | Length |
| --- | --- | --- |
| `0x0000000140002040` | `GIF-Image` | `35` |
| `0x0000000140002068` | `GIF-Image` | `35` |
| `0x0000000140002090` | `PNG-Image` | `68` |
| `0x00000001400020E0` | `JPEG-Image` | `1345` |
| `0x0000000140002628` | `WAVE-Sound` | `44` |
| `0x0000000140002658` | `MIDI-Score` | `26` |
| `0x0000000140002678` | `AU-Sound` | `24` |

## Embedded Media Bookmarks

| Offset | Comment |
| --- | --- |
| `0x0000000140002040` | `Found GIF 89 Embedded Media` |
| `0x0000000140002068` | `Found GIF 87 Embedded Media` |
| `0x0000000140002090` | `Found PNG Embedded Media` |
| `0x00000001400020E0` | `Found JPEG Embedded Media` |
| `0x0000000140002628` | `Found WAVE Embedded Media` |
| `0x0000000140002658` | `Found MIDI Embedded Media` |
| `0x0000000140002678` | `Found AU Embedded Media` |

## Fixture Assertions

- **Successful media data objects:** `7`.
- **Analyzer bookmarks:** `7`.
