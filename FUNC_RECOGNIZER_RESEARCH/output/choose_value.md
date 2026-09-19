# P-Code Execution Trace: `choose_value`

**Arguments:** `(1, 11, 22)`

## Initial State

Sparse register/temporary state is shown only when a byte location is changed.

- Program counter: `0`
- `RCX` = `0x1` (1)
- `RDX` = `0xb` (11)
- `RSP` = `0x200000` (2097152)
- `R8` = `0x16` (22)

## Execution Steps

### Step 0

**P-Code:** `INT_ADD 0x18:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `0` -> `1`

**State before/after:**
- `unique_0x9d00`: `<unmapped>` -> `0x200018 (2097176)`

### Step 1

**P-Code:** `COPY R8D:4B -> unique_0xd400:4B`

**Program counter:** `1` -> `2`

**State before/after:**
- `unique_0xd400`: `<unmapped>` -> `0x16 (22)`

### Step 2

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x200018/4B = 0x16`
**Program counter:** `2` -> `3`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x200018`: `<unmapped>` -> `0x16 (22)`
- `ram:0x200019`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20001a`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20001b`: `<unmapped>` -> `0x00 (0)`

### Step 3

**P-Code:** `INT_ADD 0x10:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `3` -> `4`

**State before/after:**
- `unique_0x9d00`: `0x200018 (2097176)` -> `0x200010 (2097168)`

### Step 4

**P-Code:** `COPY EDX:4B -> unique_0xd400:4B`

**Program counter:** `4` -> `5`

**State before/after:**
- `unique_0xd400`: `0x16 (22)` -> `0x0b (11)`

### Step 5

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x200010/4B = 0xb`
**Program counter:** `5` -> `6`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x200010`: `<unmapped>` -> `0x0b (11)`
- `ram:0x200011`: `<unmapped>` -> `0x00 (0)`
- `ram:0x200012`: `<unmapped>` -> `0x00 (0)`
- `ram:0x200013`: `<unmapped>` -> `0x00 (0)`

### Step 6

**P-Code:** `INT_ADD 0x8:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `6` -> `7`

**State before/after:**
- `unique_0x9d00`: `0x200010 (2097168)` -> `0x200008 (2097160)`

### Step 7

**P-Code:** `COPY ECX:4B -> unique_0xd400:4B`

**Program counter:** `7` -> `8`

**State before/after:**
- `unique_0xd400`: `0x0b (11)` -> `0x01 (1)`

### Step 8

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x200008/4B = 0x1`
**Program counter:** `8` -> `9`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x200008`: `<unmapped>` -> `0x01 (1)`
- `ram:0x200009`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20000a`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20000b`: `<unmapped>` -> `0x00 (0)`

### Step 9

**P-Code:** `INT_ADD 0x8:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `9` -> `10`

**State before/after:**
- No register or unique-byte changes.

### Step 10

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200008/4B = 0x1`
**Program counter:** `10` -> `11`

**State before/after:**
- `unique_0x23d00`: `<unmapped>` -> `0x01 (1)`

### Step 11

**P-Code:** `COPY unique_0x23d00:4B -> unique_0x78b00:4B`

**Program counter:** `11` -> `12`

**State before/after:**
- `unique_0x78b00`: `<unmapped>` -> `0x01 (1)`

### Step 12

**P-Code:** `INT_LESS unique_0x78b00:4B, 0x0:4B -> CF:1B`

**Program counter:** `12` -> `13`

**State before/after:**
- `CF`: `<unmapped>` -> `0x00 (0)`

### Step 13

**P-Code:** `INT_SBORROW unique_0x78b00:4B, 0x0:4B -> OF:1B`

**Program counter:** `13` -> `14`

**State before/after:**
- `OF`: `<unmapped>` -> `0x00 (0)`

### Step 14

**P-Code:** `INT_SUB unique_0x78b00:4B, 0x0:4B -> unique_0x78d00:4B`

**Program counter:** `14` -> `15`

**State before/after:**
- `unique_0x78d00`: `<unmapped>` -> `0x01 (1)`

### Step 15

**P-Code:** `INT_SLESS unique_0x78d00:4B, 0x0:4B -> SF:1B`

**Program counter:** `15` -> `16`

**State before/after:**
- `SF`: `<unmapped>` -> `0x00 (0)`

### Step 16

**P-Code:** `INT_EQUAL unique_0x78d00:4B, 0x0:4B -> ZF:1B`

**Program counter:** `16` -> `17`

**State before/after:**
- `ZF`: `<unmapped>` -> `0x00 (0)`

### Step 17

**P-Code:** `INT_AND unique_0x78d00:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `17` -> `18`

**State before/after:**
- `unique_0x58300`: `<unmapped>` -> `0x01 (1)`

### Step 18

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `18` -> `19`

**State before/after:**
- `unique_0x58400`: `<unmapped>` -> `0x01 (1)`

### Step 19

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `19` -> `20`

**State before/after:**
- `unique_0x58500`: `<unmapped>` -> `0x01 (1)`

### Step 20

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `20` -> `21`

**State before/after:**
- `PF`: `<unmapped>` -> `0x00 (0)`

### Step 21

**P-Code:** `CBRANCH ram[0x14000131a]:8B, ZF:1B`

**Control flow:** fall-through (target 27)
**Program counter:** `21` -> `22`

**State before/after:**
- No register or unique-byte changes.

### Step 22

**P-Code:** `INT_ADD 0x10:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `22` -> `23`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x200010 (2097168)`

### Step 23

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200010/4B = 0xb`
**Program counter:** `23` -> `24`

**State before/after:**
- `unique_0x23d00`: `0x01 (1)` -> `0x0b (11)`

### Step 24

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `24` -> `25`

**State before/after:**
- `EAX`: `<unmapped>` -> `0x0b (11)`

### Step 25

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `25` -> `26`

**State before/after:**
- `RAX`: `<unmapped>` -> `0x0b (11)`

### Step 26

**P-Code:** `BRANCH ram[0x14000131e]:8B`

**Control flow:** branch taken (target 31)
**Program counter:** `26` -> `31`

**State before/after:**
- No register or unique-byte changes.

### Step 27

**P-Code:** `LOAD 0x1b1:8B, RSP:8B -> RIP:8B`

**Memory reads:** `ram:0x200000/8B = 0xdeadbeef`
**Program counter:** `31` -> `32`

**State before/after:**
- `RIP`: `<unmapped>` -> `0xdeadbeef (3735928559)`

### Step 28

**P-Code:** `INT_ADD RSP:8B, 0x8:8B -> RSP:8B`

**Program counter:** `32` -> `33`

**State before/after:**
- `RSP`: `0x200000 (2097152)` -> `0x200008 (2097160)`

### Step 29

**P-Code:** `RETURN RIP:8B`

**Control flow:** return
**Program counter:** `33` -> `33`

**State before/after:**
- No register or unique-byte changes.

## Final State

- Program counter: `33`
- Halted: `True`


## Validation

**Expected result:**
`11`

**Normal C++ result:**
`11`

**Emulated result:**
`11`

**Status:** `PASS`
