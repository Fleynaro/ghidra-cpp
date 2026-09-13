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

## Before target analysis

### Data

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Functions

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Bookmarks

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Options

| Key | State |
| --- | --- |
| `Create Analysis Bookmarks` | `true` |
| `Embedded Media` | `true` |


## After target analysis

### Data

| Key | State |
| --- | --- |
| `0x0000000140002040` | `GIF-Image (35 bytes)` |
| `0x0000000140002068` | `GIF-Image (35 bytes)` |
| `0x0000000140002090` | `PNG-Image (68 bytes)` |
| `0x00000001400020E0` | `JPEG-Image (1345 bytes)` |
| `0x0000000140002628` | `WAVE-Sound (44 bytes)` |
| `0x0000000140002658` | `MIDI-Score (26 bytes)` |
| `0x0000000140002678` | `AU-Sound (24 bytes)` |

### Functions

| Key | State |
| --- | --- |
| `(none)` | `No rows observed` |

### Bookmarks

| Key | State |
| --- | --- |
| `0x0000000140002040 Embedded Media` | `Found GIF 89 Embedded Media` |
| `0x0000000140002068 Embedded Media` | `Found GIF 87 Embedded Media` |
| `0x0000000140002090 Embedded Media` | `Found PNG Embedded Media` |
| `0x00000001400020E0 Embedded Media` | `Found JPEG Embedded Media` |
| `0x0000000140002628 Embedded Media` | `Found WAVE Embedded Media` |
| `0x0000000140002658 Embedded Media` | `Found MIDI Embedded Media` |
| `0x0000000140002678 Embedded Media` | `Found AU Embedded Media` |

### Options

| Key | State |
| --- | --- |
| `Create Analysis Bookmarks` | `true` |
| `Embedded Media` | `true` |


## Delta

### Data

| Change | Key | Before | After |
| --- | --- | --- | --- |
| `Added` | `0x0000000140002040` | `` | `GIF-Image (35 bytes)` |
| `Added` | `0x0000000140002068` | `` | `GIF-Image (35 bytes)` |
| `Added` | `0x0000000140002090` | `` | `PNG-Image (68 bytes)` |
| `Added` | `0x00000001400020E0` | `` | `JPEG-Image (1345 bytes)` |
| `Added` | `0x0000000140002628` | `` | `WAVE-Sound (44 bytes)` |
| `Added` | `0x0000000140002658` | `` | `MIDI-Score (26 bytes)` |
| `Added` | `0x0000000140002678` | `` | `AU-Sound (24 bytes)` |

### Functions

No changes observed.

### Bookmarks

| Change | Key | Before | After |
| --- | --- | --- | --- |
| `Added` | `0x0000000140002040 Embedded Media` | `` | `Found GIF 89 Embedded Media` |
| `Added` | `0x0000000140002068 Embedded Media` | `` | `Found GIF 87 Embedded Media` |
| `Added` | `0x0000000140002090 Embedded Media` | `` | `Found PNG Embedded Media` |
| `Added` | `0x00000001400020E0 Embedded Media` | `` | `Found JPEG Embedded Media` |
| `Added` | `0x0000000140002628 Embedded Media` | `` | `Found WAVE Embedded Media` |
| `Added` | `0x0000000140002658 Embedded Media` | `` | `Found MIDI Embedded Media` |
| `Added` | `0x0000000140002678 Embedded Media` | `` | `Found AU Embedded Media` |

### Options

No changes observed.

## Fixture Assertions

- **Successful media data objects:** `7`.
- **Analyzer bookmarks:** `7`.
