# P-Code Execution Trace: `multiply`

**Arguments:** `(7, 6)`

## Initial State

Sparse register/temporary state is shown only when a byte location is changed.

- Program counter: `0`
- `RCX` = `0x7` (7)
- `RDX` = `0x6` (6)
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
- `unique_0xd400`: `<unmapped>` -> `0x06 (6)`

### Step 2

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x200010/4B = 0x6`
**Program counter:** `2` -> `3`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x200010`: `<unmapped>` -> `0x06 (6)`
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
- `unique_0xd400`: `0x06 (6)` -> `0x07 (7)`

### Step 5

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x200008/4B = 0x7`
**Program counter:** `5` -> `6`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x200008`: `<unmapped>` -> `0x07 (7)`
- `ram:0x200009`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20000a`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20000b`: `<unmapped>` -> `0x00 (0)`

### Step 6

**P-Code:** `INT_LESS RSP:8B, 0x18:8B -> CF:1B`

**Program counter:** `6` -> `7`

**State before/after:**
- `CF`: `<unmapped>` -> `0x00 (0)`

### Step 7

**P-Code:** `INT_SBORROW RSP:8B, 0x18:8B -> OF:1B`

**Program counter:** `7` -> `8`

**State before/after:**
- `OF`: `<unmapped>` -> `0x00 (0)`

### Step 8

**P-Code:** `INT_SUB RSP:8B, 0x18:8B -> RSP:8B`

**Program counter:** `8` -> `9`

**State before/after:**
- `RSP`: `0x200000 (2097152)` -> `0x1fffe8 (2097128)`

### Step 9

**P-Code:** `INT_SLESS RSP:8B, 0x0:8B -> SF:1B`

**Program counter:** `9` -> `10`

**State before/after:**
- `SF`: `<unmapped>` -> `0x00 (0)`

### Step 10

**P-Code:** `INT_EQUAL RSP:8B, 0x0:8B -> ZF:1B`

**Program counter:** `10` -> `11`

**State before/after:**
- `ZF`: `<unmapped>` -> `0x00 (0)`

### Step 11

**P-Code:** `INT_AND RSP:8B, 0xff:8B -> unique_0x58300:8B`

**Program counter:** `11` -> `12`

**State before/after:**
- `unique_0x58300`: `<unmapped>` -> `0xe8 (232)`

### Step 12

**P-Code:** `POPCOUNT unique_0x58300:8B -> unique_0x58400:1B`

**Program counter:** `12` -> `13`

**State before/after:**
- `unique_0x58400`: `<unmapped>` -> `0x04 (4)`

### Step 13

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `13` -> `14`

**State before/after:**
- `unique_0x58500`: `<unmapped>` -> `0x00 (0)`

### Step 14

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `14` -> `15`

**State before/after:**
- `PF`: `<unmapped>` -> `0x01 (1)`

### Step 15

**P-Code:** `INT_ADD 0x20:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `15` -> `16`

**State before/after:**
- No register or unique-byte changes.

### Step 16

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200008/4B = 0x7`
**Program counter:** `16` -> `17`

**State before/after:**
- `unique_0x23d00`: `<unmapped>` -> `0x07 (7)`

### Step 17

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `17` -> `18`

**State before/after:**
- `EAX`: `<unmapped>` -> `0x07 (7)`

### Step 18

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `18` -> `19`

**State before/after:**
- `RAX`: `<unmapped>` -> `0x07 (7)`

### Step 19

**P-Code:** `INT_ADD 0x28:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `19` -> `20`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x200010 (2097168)`

### Step 20

**P-Code:** `INT_SEXT EAX:4B -> unique_0x92500:8B`

**Program counter:** `20` -> `21`

**State before/after:**
- `unique_0x92500`: `<unmapped>` -> `0x07 (7)`

### Step 21

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200010/4B = 0x6`
**Program counter:** `21` -> `22`

**State before/after:**
- `unique_0x23d00`: `0x07 (7)` -> `0x06 (6)`

### Step 22

**P-Code:** `INT_SEXT unique_0x23d00:4B -> unique_0x92600:8B`

**Program counter:** `22` -> `23`

**State before/after:**
- `unique_0x92600`: `<unmapped>` -> `0x06 (6)`

### Step 23

**P-Code:** `INT_MULT unique_0x92500:8B, unique_0x92600:8B -> unique_0x92800:8B`

**Program counter:** `23` -> `24`

**State before/after:**
- `unique_0x92800`: `<unmapped>` -> `0x2a (42)`

### Step 24

**P-Code:** `SUBPIECE unique_0x92800:8B, 0x0:4B -> EAX:4B`

**Program counter:** `24` -> `25`

**State before/after:**
- `EAX`: `0x07 (7)` -> `0x2a (42)`
- `RAX`: `0x07 (7)` -> `0x2a (42)`

### Step 25

**P-Code:** `SUBPIECE unique_0x92800:8B, 0x4:4B -> unique_0x92b00:4B`

**Program counter:** `25` -> `26`

**State before/after:**
- `unique_0x92b00`: `<unmapped>` -> `0x00 (0)`

### Step 26

**P-Code:** `INT_SEXT EAX:4B -> unique_0x5a200:8B`

**Program counter:** `26` -> `27`

**State before/after:**
- `unique_0x5a200`: `<unmapped>` -> `0x2a (42)`

### Step 27

**P-Code:** `INT_NOTEQUAL unique_0x5a200:8B, unique_0x92800:8B -> CF:1B`

**Program counter:** `27` -> `28`

**State before/after:**
- No register or unique-byte changes.

### Step 28

**P-Code:** `COPY CF:1B -> OF:1B`

**Program counter:** `28` -> `29`

**State before/after:**
- No register or unique-byte changes.

### Step 29

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `29` -> `30`

**State before/after:**
- No register or unique-byte changes.

### Step 30

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `30` -> `31`

**State before/after:**
- `unique_0xd400`: `0x07 (7)` -> `0x2a (42)`

### Step 31

**P-Code:** `STORE 0x1b1:4B, RSP:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffe8/4B = 0x2a`
**Program counter:** `31` -> `32`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffe8`: `<unmapped>` -> `0x2a (42)`
- `ram:0x1fffe9`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffea`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffeb`: `<unmapped>` -> `0x00 (0)`

### Step 32

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x2a`
**Program counter:** `32` -> `33`

**State before/after:**
- `unique_0x23d00`: `0x06 (6)` -> `0x2a (42)`

### Step 33

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `33` -> `34`

**State before/after:**
- No register or unique-byte changes.

### Step 34

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `34` -> `35`

**State before/after:**
- No register or unique-byte changes.

### Step 35

**P-Code:** `INT_CARRY RSP:8B, 0x18:8B -> CF:1B`

**Program counter:** `35` -> `36`

**State before/after:**
- No register or unique-byte changes.

### Step 36

**P-Code:** `INT_SCARRY RSP:8B, 0x18:8B -> OF:1B`

**Program counter:** `36` -> `37`

**State before/after:**
- No register or unique-byte changes.

### Step 37

**P-Code:** `INT_ADD RSP:8B, 0x18:8B -> RSP:8B`

**Program counter:** `37` -> `38`

**State before/after:**
- `RSP`: `0x1fffe8 (2097128)` -> `0x200000 (2097152)`

### Step 38

**P-Code:** `INT_SLESS RSP:8B, 0x0:8B -> SF:1B`

**Program counter:** `38` -> `39`

**State before/after:**
- No register or unique-byte changes.

### Step 39

**P-Code:** `INT_EQUAL RSP:8B, 0x0:8B -> ZF:1B`

**Program counter:** `39` -> `40`

**State before/after:**
- No register or unique-byte changes.

### Step 40

**P-Code:** `INT_AND RSP:8B, 0xff:8B -> unique_0x58300:8B`

**Program counter:** `40` -> `41`

**State before/after:**
- `unique_0x58300`: `0xe8 (232)` -> `0x00 (0)`

### Step 41

**P-Code:** `POPCOUNT unique_0x58300:8B -> unique_0x58400:1B`

**Program counter:** `41` -> `42`

**State before/after:**
- `unique_0x58400`: `0x04 (4)` -> `0x00 (0)`

### Step 42

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `42` -> `43`

**State before/after:**
- No register or unique-byte changes.

### Step 43

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `43` -> `44`

**State before/after:**
- No register or unique-byte changes.

### Step 44

**P-Code:** `LOAD 0x1b1:8B, RSP:8B -> RIP:8B`

**Memory reads:** `ram:0x200000/8B = 0xdeadbeef`
**Program counter:** `44` -> `45`

**State before/after:**
- `RIP`: `<unmapped>` -> `0xdeadbeef (3735928559)`

### Step 45

**P-Code:** `INT_ADD RSP:8B, 0x8:8B -> RSP:8B`

**Program counter:** `45` -> `46`

**State before/after:**
- `RSP`: `0x200000 (2097152)` -> `0x200008 (2097160)`

### Step 46

**P-Code:** `RETURN RIP:8B`

**Control flow:** return
**Program counter:** `46` -> `46`

**State before/after:**
- No register or unique-byte changes.

## Final State

- Program counter: `46`
- Halted: `True`


## Validation

**Expected result:**
`42`

**Normal C++ result:**
`42`

**Emulated result:**
`42`

**Status:** `PASS`
