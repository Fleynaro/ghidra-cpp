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
| `0x0000000140002070` | `PNG-Image` | `68` |
| `0x00000001400020C0` | `JPEG-Image` | `1345` |
| `0x0000000140002608` | `WAVE-Sound` | `44` |
| `0x0000000140002638` | `MIDI-Score` | `26` |
| `0x0000000140002658` | `AU-Sound` | `24` |

## Embedded Media Bookmarks

| Offset | Comment |
| --- | --- |
| `0x0000000140002040` | `Found GIF 89 Embedded Media` |
| `0x0000000140002070` | `Found PNG Embedded Media` |
| `0x00000001400020C0` | `Found JPEG Embedded Media` |
| `0x0000000140002608` | `Found WAVE Embedded Media` |
| `0x0000000140002638` | `Found MIDI Embedded Media` |
| `0x0000000140002658` | `Found AU Embedded Media` |

## Fixture Assertions

- **Successful media data objects:** `6`.
- **Analyzer bookmarks:** `6`.
