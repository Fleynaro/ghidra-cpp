# P-Code Execution Trace: `absolute_difference`

**Arguments:** `(9, 23)`

## Initial State

Sparse register/temporary state is shown only when a byte location is changed.

- Program counter: `0`
- `RCX` = `0x9` (9)
- `RDX` = `0x17` (23)
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
- `unique_0xd400`: `<unmapped>` -> `0x17 (23)`

### Step 2

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x200010/4B = 0x17`
**Program counter:** `2` -> `3`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x200010`: `<unmapped>` -> `0x17 (23)`
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
- `unique_0xd400`: `0x17 (23)` -> `0x09 (9)`

### Step 5

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x200008/4B = 0x9`
**Program counter:** `5` -> `6`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x200008`: `<unmapped>` -> `0x09 (9)`
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

**P-Code:** `INT_ADD 0x28:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `15` -> `16`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x200010 (2097168)`

### Step 16

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200010/4B = 0x17`
**Program counter:** `16` -> `17`

**State before/after:**
- `unique_0x23d00`: `<unmapped>` -> `0x17 (23)`

### Step 17

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `17` -> `18`

**State before/after:**
- `EAX`: `<unmapped>` -> `0x17 (23)`

### Step 18

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `18` -> `19`

**State before/after:**
- `RAX`: `<unmapped>` -> `0x17 (23)`

### Step 19

**P-Code:** `INT_ADD 0x20:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `19` -> `20`

**State before/after:**
- `unique_0x9d00`: `0x200010 (2097168)` -> `0x200008 (2097160)`

### Step 20

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200008/4B = 0x9`
**Program counter:** `20` -> `21`

**State before/after:**
- `unique_0x23d00`: `0x17 (23)` -> `0x09 (9)`

### Step 21

**P-Code:** `COPY unique_0x23d00:4B -> ECX:4B`

**Program counter:** `21` -> `22`

**State before/after:**
- `ECX`: `<unmapped>` -> `0x09 (9)`

### Step 22

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `22` -> `23`

**State before/after:**
- No register or unique-byte changes.

### Step 23

**P-Code:** `INT_LESS ECX:4B, EAX:4B -> CF:1B`

**Program counter:** `23` -> `24`

**State before/after:**
- `CF`: `0x00 (0)` -> `0x01 (1)`

### Step 24

**P-Code:** `INT_SBORROW ECX:4B, EAX:4B -> OF:1B`

**Program counter:** `24` -> `25`

**State before/after:**
- No register or unique-byte changes.

### Step 25

**P-Code:** `INT_SUB ECX:4B, EAX:4B -> ECX:4B`

**Program counter:** `25` -> `26`

**State before/after:**
- `ECX`: `0x09 (9)` -> `0xfffffff2 (4294967282)`
- `RCX`: `0x09 (9)` -> `0xfffffff2 (4294967282)`

### Step 26

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `26` -> `27`

**State before/after:**
- No register or unique-byte changes.

### Step 27

**P-Code:** `INT_SLESS ECX:4B, 0x0:4B -> SF:1B`

**Program counter:** `27` -> `28`

**State before/after:**
- `SF`: `0x00 (0)` -> `0x01 (1)`

### Step 28

**P-Code:** `INT_EQUAL ECX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `28` -> `29`

**State before/after:**
- No register or unique-byte changes.

### Step 29

**P-Code:** `INT_AND ECX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `29` -> `30`

**State before/after:**
- `unique_0x58300`: `<unmapped>` -> `0xf2 (242)`
- `unique_0x58300`: `0xe8 (232)` -> `0xf2 (242)`

### Step 30

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `30` -> `31`

**State before/after:**
- `unique_0x58400`: `0x04 (4)` -> `0x05 (5)`

### Step 31

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `31` -> `32`

**State before/after:**
- `unique_0x58500`: `0x00 (0)` -> `0x01 (1)`

### Step 32

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `32` -> `33`

**State before/after:**
- `PF`: `0x01 (1)` -> `0x00 (0)`

### Step 33

**P-Code:** `COPY ECX:4B -> EAX:4B`

**Program counter:** `33` -> `34`

**State before/after:**
- `EAX`: `0x17 (23)` -> `0xfffffff2 (4294967282)`
- `RAX`: `0x17 (23)` -> `0xfffffff2 (4294967282)`

### Step 34

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `34` -> `35`

**State before/after:**
- No register or unique-byte changes.

### Step 35

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `35` -> `36`

**State before/after:**
- `unique_0xd400`: `0x09 (9)` -> `0xfffffff2 (4294967282)`

### Step 36

**P-Code:** `STORE 0x1b1:4B, RSP:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffe8/4B = 0xfffffff2`
**Program counter:** `36` -> `37`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffe8`: `<unmapped>` -> `0xf2 (242)`
- `ram:0x1fffe9`: `<unmapped>` -> `0xff (255)`
- `ram:0x1fffea`: `<unmapped>` -> `0xff (255)`
- `ram:0x1fffeb`: `<unmapped>` -> `0xff (255)`

### Step 37

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0xfffffff2`
**Program counter:** `37` -> `38`

**State before/after:**
- `unique_0x23d00`: `0x09 (9)` -> `0xfffffff2 (4294967282)`

### Step 38

**P-Code:** `COPY unique_0x23d00:4B -> unique_0x78b00:4B`

**Program counter:** `38` -> `39`

**State before/after:**
- `unique_0x78b00`: `<unmapped>` -> `0xfffffff2 (4294967282)`

### Step 39

**P-Code:** `INT_LESS unique_0x78b00:4B, 0x0:4B -> CF:1B`

**Program counter:** `39` -> `40`

**State before/after:**
- `CF`: `0x01 (1)` -> `0x00 (0)`

### Step 40

**P-Code:** `INT_SBORROW unique_0x78b00:4B, 0x0:4B -> OF:1B`

**Program counter:** `40` -> `41`

**State before/after:**
- No register or unique-byte changes.

### Step 41

**P-Code:** `INT_SUB unique_0x78b00:4B, 0x0:4B -> unique_0x78d00:4B`

**Program counter:** `41` -> `42`

**State before/after:**
- `unique_0x78d00`: `<unmapped>` -> `0xfffffff2 (4294967282)`

### Step 42

**P-Code:** `INT_SLESS unique_0x78d00:4B, 0x0:4B -> SF:1B`

**Program counter:** `42` -> `43`

**State before/after:**
- No register or unique-byte changes.

### Step 43

**P-Code:** `INT_EQUAL unique_0x78d00:4B, 0x0:4B -> ZF:1B`

**Program counter:** `43` -> `44`

**State before/after:**
- No register or unique-byte changes.

### Step 44

**P-Code:** `INT_AND unique_0x78d00:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `44` -> `45`

**State before/after:**
- No register or unique-byte changes.

### Step 45

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `45` -> `46`

**State before/after:**
- No register or unique-byte changes.

### Step 46

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `46` -> `47`

**State before/after:**
- No register or unique-byte changes.

### Step 47

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `47` -> `48`

**State before/after:**
- No register or unique-byte changes.

### Step 48

**P-Code:** `INT_EQUAL OF:1B, SF:1B -> unique_0x25c00:1B`

**Program counter:** `48` -> `49`

**State before/after:**
- `unique_0x25c00`: `<unmapped>` -> `0x00 (0)`

### Step 49

**P-Code:** `CBRANCH ram[0x14000113c]:8B, unique_0x25c00:1B`

**Control flow:** fall-through (target 67)
**Program counter:** `49` -> `50`

**State before/after:**
- No register or unique-byte changes.

### Step 50

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0xfffffff2`
**Program counter:** `50` -> `51`

**State before/after:**
- No register or unique-byte changes.

### Step 51

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `51` -> `52`

**State before/after:**
- No register or unique-byte changes.

### Step 52

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `52` -> `53`

**State before/after:**
- No register or unique-byte changes.

### Step 53

**P-Code:** `INT_NOTEQUAL EAX:4B, 0x0:4B -> CF:1B`

**Program counter:** `53` -> `54`

**State before/after:**
- `CF`: `0x00 (0)` -> `0x01 (1)`

### Step 54

**P-Code:** `INT_SBORROW 0x0:4B, EAX:4B -> OF:1B`

**Program counter:** `54` -> `55`

**State before/after:**
- No register or unique-byte changes.

### Step 55

**P-Code:** `INT_2COMP EAX:4B -> EAX:4B`

**Program counter:** `55` -> `56`

**State before/after:**
- `EAX`: `0xfffffff2 (4294967282)` -> `0x0e (14)`
- `RAX`: `0xfffffff2 (4294967282)` -> `0x0e (14)`

### Step 56

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `56` -> `57`

**State before/after:**
- `SF`: `0x01 (1)` -> `0x00 (0)`

### Step 57

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `57` -> `58`

**State before/after:**
- No register or unique-byte changes.

### Step 58

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `58` -> `59`

**State before/after:**
- `unique_0x58300`: `0xf2 (242)` -> `0x0e (14)`
- `unique_0x58300`: `0xf2 (242)` -> `0x0e (14)`

### Step 59

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `59` -> `60`

**State before/after:**
- `unique_0x58400`: `0x05 (5)` -> `0x03 (3)`

### Step 60

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `60` -> `61`

**State before/after:**
- No register or unique-byte changes.

### Step 61

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `61` -> `62`

**State before/after:**
- No register or unique-byte changes.

### Step 62

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `62` -> `63`

**State before/after:**
- No register or unique-byte changes.

### Step 63

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `63` -> `64`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x1fffec (2097132)`

### Step 64

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `64` -> `65`

**State before/after:**
- `unique_0xd400`: `0xfffffff2 (4294967282)` -> `0x0e (14)`

### Step 65

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffec/4B = 0xe`
**Program counter:** `65` -> `66`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffec`: `<unmapped>` -> `0x0e (14)`
- `ram:0x1fffed`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffee`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffef`: `<unmapped>` -> `0x00 (0)`

### Step 66

**P-Code:** `BRANCH ram[0x140001143]:8B`

**Control flow:** branch taken (target 73)
**Program counter:** `66` -> `73`

**State before/after:**
- No register or unique-byte changes.

### Step 67

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `73` -> `74`

**State before/after:**
- No register or unique-byte changes.

### Step 68

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffec/4B = 0xe`
**Program counter:** `74` -> `75`

**State before/after:**
- `unique_0x23d00`: `0xfffffff2 (4294967282)` -> `0x0e (14)`

### Step 69

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `75` -> `76`

**State before/after:**
- No register or unique-byte changes.

### Step 70

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `76` -> `77`

**State before/after:**
- No register or unique-byte changes.

### Step 71

**P-Code:** `INT_CARRY RSP:8B, 0x18:8B -> CF:1B`

**Program counter:** `77` -> `78`

**State before/after:**
- `CF`: `0x01 (1)` -> `0x00 (0)`

### Step 72

**P-Code:** `INT_SCARRY RSP:8B, 0x18:8B -> OF:1B`

**Program counter:** `78` -> `79`

**State before/after:**
- No register or unique-byte changes.

### Step 73

**P-Code:** `INT_ADD RSP:8B, 0x18:8B -> RSP:8B`

**Program counter:** `79` -> `80`

**State before/after:**
- `RSP`: `0x1fffe8 (2097128)` -> `0x200000 (2097152)`

### Step 74

**P-Code:** `INT_SLESS RSP:8B, 0x0:8B -> SF:1B`

**Program counter:** `80` -> `81`

**State before/after:**
- No register or unique-byte changes.

### Step 75

**P-Code:** `INT_EQUAL RSP:8B, 0x0:8B -> ZF:1B`

**Program counter:** `81` -> `82`

**State before/after:**
- No register or unique-byte changes.

### Step 76

**P-Code:** `INT_AND RSP:8B, 0xff:8B -> unique_0x58300:8B`

**Program counter:** `82` -> `83`

**State before/after:**
- `unique_0x58300`: `0x0e (14)` -> `0x00 (0)`
- `unique_0x58300`: `0x0e (14)` -> `0x00 (0)`

### Step 77

**P-Code:** `POPCOUNT unique_0x58300:8B -> unique_0x58400:1B`

**Program counter:** `83` -> `84`

**State before/after:**
- `unique_0x58400`: `0x03 (3)` -> `0x00 (0)`

### Step 78

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `84` -> `85`

**State before/after:**
- `unique_0x58500`: `0x01 (1)` -> `0x00 (0)`

### Step 79

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `85` -> `86`

**State before/after:**
- `PF`: `0x00 (0)` -> `0x01 (1)`

### Step 80

**P-Code:** `LOAD 0x1b1:8B, RSP:8B -> RIP:8B`

**Memory reads:** `ram:0x200000/8B = 0xdeadbeef`
**Program counter:** `86` -> `87`

**State before/after:**
- `RIP`: `<unmapped>` -> `0xdeadbeef (3735928559)`

### Step 81

**P-Code:** `INT_ADD RSP:8B, 0x8:8B -> RSP:8B`

**Program counter:** `87` -> `88`

**State before/after:**
- `RSP`: `0x200000 (2097152)` -> `0x200008 (2097160)`

### Step 82

**P-Code:** `RETURN RIP:8B`

**Control flow:** return
**Program counter:** `88` -> `88`

**State before/after:**
- No register or unique-byte changes.

## Final State

- Program counter: `88`
- Halted: `True`


## Validation

**Expected result:**
`14`

**Normal C++ result:**
`14`

**Emulated result:**
`14`

**Status:** `PASS`
