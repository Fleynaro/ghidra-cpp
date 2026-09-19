# P-Code Execution Trace: `add`

**Arguments:** `(10, 32)`

## Initial State

Sparse register/temporary state is shown only when a byte location is changed.

- Program counter: `0`
- `RCX` = `0xa` (10)
- `RDX` = `0x20` (32)
- `RSP` = `0x200000` (2097152)

## Execution Steps

### Step 0

**P-Code:** `INT_ADD 0x10:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `0` -> `1`

**State before/after:**
- `unique_0x9d00`: `<unmapped>` -> `0x200010 (2097168)`

### Step 1

**P-Code:** `COPY EDX:4B -> unique_0xd400:4B`

**Program counter:** `1` -> `2`

**State before/after:**
- `unique_0xd400`: `<unmapped>` -> `0x20 (32)`

### Step 2

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x200010/4B = 0x20`
**Program counter:** `2` -> `3`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x200010`: `<unmapped>` -> `0x20 (32)`
- `ram:0x200011`: `<unmapped>` -> `0x00 (0)`
- `ram:0x200012`: `<unmapped>` -> `0x00 (0)`
- `ram:0x200013`: `<unmapped>` -> `0x00 (0)`

### Step 3

**P-Code:** `INT_ADD 0x8:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `3` -> `4`

**State before/after:**
- `unique_0x9d00`: `0x200010 (2097168)` -> `0x200008 (2097160)`

### Step 4

**P-Code:** `COPY ECX:4B -> unique_0xd400:4B`

**Program counter:** `4` -> `5`

**State before/after:**
- `unique_0xd400`: `0x20 (32)` -> `0x0a (10)`

### Step 5

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x200008/4B = 0xa`
**Program counter:** `5` -> `6`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x200008`: `<unmapped>` -> `0x0a (10)`
- `ram:0x200009`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20000a`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20000b`: `<unmapped>` -> `0x00 (0)`

### Step 6

**P-Code:** `INT_ADD 0x10:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `6` -> `7`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x200010 (2097168)`

### Step 7

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200010/4B = 0x20`
**Program counter:** `7` -> `8`

**State before/after:**
- `unique_0x23d00`: `<unmapped>` -> `0x20 (32)`

### Step 8

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `8` -> `9`

**State before/after:**
- `EAX`: `<unmapped>` -> `0x20 (32)`

### Step 9

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `9` -> `10`

**State before/after:**
- `RAX`: `<unmapped>` -> `0x20 (32)`

### Step 10

**P-Code:** `INT_ADD 0x8:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `10` -> `11`

**State before/after:**
- `unique_0x9d00`: `0x200010 (2097168)` -> `0x200008 (2097160)`

### Step 11

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200008/4B = 0xa`
**Program counter:** `11` -> `12`

**State before/after:**
- `unique_0x23d00`: `0x20 (32)` -> `0x0a (10)`

### Step 12

**P-Code:** `COPY unique_0x23d00:4B -> ECX:4B`

**Program counter:** `12` -> `13`

**State before/after:**
- `ECX`: `<unmapped>` -> `0x0a (10)`

### Step 13

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `13` -> `14`

**State before/after:**
- No register or unique-byte changes.

### Step 14

**P-Code:** `INT_CARRY ECX:4B, EAX:4B -> CF:1B`

**Program counter:** `14` -> `15`

**State before/after:**
- `CF`: `<unmapped>` -> `0x00 (0)`

### Step 15

**P-Code:** `INT_SCARRY ECX:4B, EAX:4B -> OF:1B`

**Program counter:** `15` -> `16`

**State before/after:**
- `OF`: `<unmapped>` -> `0x00 (0)`

### Step 16

**P-Code:** `INT_ADD ECX:4B, EAX:4B -> ECX:4B`

**Program counter:** `16` -> `17`

**State before/after:**
- `ECX`: `0x0a (10)` -> `0x2a (42)`
- `RCX`: `0x0a (10)` -> `0x2a (42)`

### Step 17

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `17` -> `18`

**State before/after:**
- No register or unique-byte changes.

### Step 18

**P-Code:** `INT_SLESS ECX:4B, 0x0:4B -> SF:1B`

**Program counter:** `18` -> `19`

**State before/after:**
- `SF`: `<unmapped>` -> `0x00 (0)`

### Step 19

**P-Code:** `INT_EQUAL ECX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `19` -> `20`

**State before/after:**
- `ZF`: `<unmapped>` -> `0x00 (0)`

### Step 20

**P-Code:** `INT_AND ECX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `20` -> `21`

**State before/after:**
- `unique_0x58300`: `<unmapped>` -> `0x2a (42)`

### Step 21

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `21` -> `22`

**State before/after:**
- `unique_0x58400`: `<unmapped>` -> `0x03 (3)`

### Step 22

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `22` -> `23`

**State before/after:**
- `unique_0x58500`: `<unmapped>` -> `0x01 (1)`

### Step 23

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `23` -> `24`

**State before/after:**
- `PF`: `<unmapped>` -> `0x00 (0)`

### Step 24

**P-Code:** `COPY ECX:4B -> EAX:4B`

**Program counter:** `24` -> `25`

**State before/after:**
- `EAX`: `0x20 (32)` -> `0x2a (42)`
- `RAX`: `0x20 (32)` -> `0x2a (42)`

### Step 25

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `25` -> `26`

**State before/after:**
- No register or unique-byte changes.

### Step 26

**P-Code:** `LOAD 0x1b1:8B, RSP:8B -> RIP:8B`

**Memory reads:** `ram:0x200000/8B = 0xdeadbeef`
**Program counter:** `26` -> `27`

**State before/after:**
- `RIP`: `<unmapped>` -> `0xdeadbeef (3735928559)`

### Step 27

**P-Code:** `INT_ADD RSP:8B, 0x8:8B -> RSP:8B`

**Program counter:** `27` -> `28`

**State before/after:**
- `RSP`: `0x200000 (2097152)` -> `0x200008 (2097160)`

### Step 28

**P-Code:** `RETURN RIP:8B`

**Control flow:** return
**Program counter:** `28` -> `28`

**State before/after:**
- No register or unique-byte changes.

## Final State

- Program counter: `28`
- Halted: `True`


## Validation

**Expected result:**
`42`

**Normal C++ result:**
`42`

**Emulated result:**
`42`

**Status:** `PASS`
