# P-Code Execution Trace: `dot_product`

**Arguments:** `(268435456, 268435712, 3)`

## Initial State

Sparse register/temporary state is shown only when a byte location is changed.

- Program counter: `0`
- `RCX` = `0x10000000` (268435456)
- `RDX` = `0x10000100` (268435712)
- `RSP` = `0x200000` (2097152)
- `R8` = `0x3` (3)
- `int[3] left` @ `0x10000000` (12 bytes): `1, 2, 3`
- `int[3] right` @ `0x10000100` (12 bytes): `4, 5, 6`

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
- `unique_0xd400`: `<unmapped>` -> `0x03 (3)`

### Step 2

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x200018/4B = 0x3`
**Program counter:** `2` -> `3`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x200018`: `<unmapped>` -> `0x03 (3)`
- `ram:0x200019`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20001a`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20001b`: `<unmapped>` -> `0x00 (0)`

### Step 3

**P-Code:** `INT_ADD 0x10:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `3` -> `4`

**State before/after:**
- `unique_0x9d00`: `0x200018 (2097176)` -> `0x200010 (2097168)`

### Step 4

**P-Code:** `COPY RDX:8B -> unique_0xd500:8B`

**Program counter:** `4` -> `5`

**State before/after:**
- `unique_0xd500`: `<unmapped>` -> `0x10000100 (268435712)`

### Step 5

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd500:8B`

**Memory writes:** `ram:0x200010/8B = 0x10000100`
**Program counter:** `5` -> `6`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x200010`: `<unmapped>` -> `0x00 (0)`
- `ram:0x200011`: `<unmapped>` -> `0x01 (1)`
- `ram:0x200012`: `<unmapped>` -> `0x00 (0)`
- `ram:0x200013`: `<unmapped>` -> `0x10 (16)`
- `ram:0x200014`: `<unmapped>` -> `0x00 (0)`
- `ram:0x200015`: `<unmapped>` -> `0x00 (0)`
- `ram:0x200016`: `<unmapped>` -> `0x00 (0)`
- `ram:0x200017`: `<unmapped>` -> `0x00 (0)`

### Step 6

**P-Code:** `INT_ADD 0x8:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `6` -> `7`

**State before/after:**
- `unique_0x9d00`: `0x200010 (2097168)` -> `0x200008 (2097160)`

### Step 7

**P-Code:** `COPY RCX:8B -> unique_0xd500:8B`

**Program counter:** `7` -> `8`

**State before/after:**
- `unique_0xd500`: `0x10000100 (268435712)` -> `0x10000000 (268435456)`

### Step 8

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd500:8B`

**Memory writes:** `ram:0x200008/8B = 0x10000000`
**Program counter:** `8` -> `9`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x200008`: `<unmapped>` -> `0x00 (0)`
- `ram:0x200009`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20000a`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20000b`: `<unmapped>` -> `0x10 (16)`
- `ram:0x20000c`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20000d`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20000e`: `<unmapped>` -> `0x00 (0)`
- `ram:0x20000f`: `<unmapped>` -> `0x00 (0)`

### Step 9

**P-Code:** `INT_LESS RSP:8B, 0x18:8B -> CF:1B`

**Program counter:** `9` -> `10`

**State before/after:**
- `CF`: `<unmapped>` -> `0x00 (0)`

### Step 10

**P-Code:** `INT_SBORROW RSP:8B, 0x18:8B -> OF:1B`

**Program counter:** `10` -> `11`

**State before/after:**
- `OF`: `<unmapped>` -> `0x00 (0)`

### Step 11

**P-Code:** `INT_SUB RSP:8B, 0x18:8B -> RSP:8B`

**Program counter:** `11` -> `12`

**State before/after:**
- `RSP`: `0x200000 (2097152)` -> `0x1fffe8 (2097128)`

### Step 12

**P-Code:** `INT_SLESS RSP:8B, 0x0:8B -> SF:1B`

**Program counter:** `12` -> `13`

**State before/after:**
- `SF`: `<unmapped>` -> `0x00 (0)`

### Step 13

**P-Code:** `INT_EQUAL RSP:8B, 0x0:8B -> ZF:1B`

**Program counter:** `13` -> `14`

**State before/after:**
- `ZF`: `<unmapped>` -> `0x00 (0)`

### Step 14

**P-Code:** `INT_AND RSP:8B, 0xff:8B -> unique_0x58300:8B`

**Program counter:** `14` -> `15`

**State before/after:**
- `unique_0x58300`: `<unmapped>` -> `0xe8 (232)`

### Step 15

**P-Code:** `POPCOUNT unique_0x58300:8B -> unique_0x58400:1B`

**Program counter:** `15` -> `16`

**State before/after:**
- `unique_0x58400`: `<unmapped>` -> `0x04 (4)`

### Step 16

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `16` -> `17`

**State before/after:**
- `unique_0x58500`: `<unmapped>` -> `0x00 (0)`

### Step 17

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `17` -> `18`

**State before/after:**
- `PF`: `<unmapped>` -> `0x01 (1)`

### Step 18

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `18` -> `19`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x1fffec (2097132)`

### Step 19

**P-Code:** `COPY 0x0:4B -> unique_0xd400:4B`

**Program counter:** `19` -> `20`

**State before/after:**
- `unique_0xd400`: `0x03 (3)` -> `0x00 (0)`

### Step 20

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffec/4B = 0x0`
**Program counter:** `20` -> `21`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffec`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffed`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffee`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffef`: `<unmapped>` -> `0x00 (0)`

### Step 21

**P-Code:** `COPY 0x0:4B -> unique_0xd400:4B`

**Program counter:** `21` -> `22`

**State before/after:**
- No register or unique-byte changes.

### Step 22

**P-Code:** `STORE 0x1b1:4B, RSP:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffe8/4B = 0x0`
**Program counter:** `22` -> `23`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffe8`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffe9`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffea`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffeb`: `<unmapped>` -> `0x00 (0)`

### Step 23

**P-Code:** `BRANCH ram[0x14000122c]:8B`

**Control flow:** branch taken (target 38)
**Program counter:** `23` -> `38`

**State before/after:**
- No register or unique-byte changes.

### Step 24

**P-Code:** `INT_ADD 0x30:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `38` -> `39`

**State before/after:**
- `unique_0x9d00`: `0x1fffec (2097132)` -> `0x200018 (2097176)`

### Step 25

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200018/4B = 0x3`
**Program counter:** `39` -> `40`

**State before/after:**
- `unique_0x23d00`: `<unmapped>` -> `0x03 (3)`

### Step 26

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `40` -> `41`

**State before/after:**
- `EAX`: `<unmapped>` -> `0x03 (3)`

### Step 27

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `41` -> `42`

**State before/after:**
- `RAX`: `<unmapped>` -> `0x03 (3)`

### Step 28

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x0`
**Program counter:** `42` -> `43`

**State before/after:**
- `unique_0x23d00`: `0x03 (3)` -> `0x00 (0)`

### Step 29

**P-Code:** `COPY unique_0x23d00:4B -> unique_0x79700:4B`

**Program counter:** `43` -> `44`

**State before/after:**
- `unique_0x79700`: `<unmapped>` -> `0x00 (0)`

### Step 30

**P-Code:** `INT_LESS unique_0x79700:4B, EAX:4B -> CF:1B`

**Program counter:** `44` -> `45`

**State before/after:**
- `CF`: `0x00 (0)` -> `0x01 (1)`

### Step 31

**P-Code:** `INT_SBORROW unique_0x79700:4B, EAX:4B -> OF:1B`

**Program counter:** `45` -> `46`

**State before/after:**
- No register or unique-byte changes.

### Step 32

**P-Code:** `INT_SUB unique_0x79700:4B, EAX:4B -> unique_0x79900:4B`

**Program counter:** `46` -> `47`

**State before/after:**
- `unique_0x79900`: `<unmapped>` -> `0xfffffffd (4294967293)`

### Step 33

**P-Code:** `INT_SLESS unique_0x79900:4B, 0x0:4B -> SF:1B`

**Program counter:** `47` -> `48`

**State before/after:**
- `SF`: `0x00 (0)` -> `0x01 (1)`

### Step 34

**P-Code:** `INT_EQUAL unique_0x79900:4B, 0x0:4B -> ZF:1B`

**Program counter:** `48` -> `49`

**State before/after:**
- No register or unique-byte changes.

### Step 35

**P-Code:** `INT_AND unique_0x79900:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `49` -> `50`

**State before/after:**
- `unique_0x58300`: `<unmapped>` -> `0xfd (253)`
- `unique_0x58300`: `0xe8 (232)` -> `0xfd (253)`

### Step 36

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `50` -> `51`

**State before/after:**
- `unique_0x58400`: `0x04 (4)` -> `0x07 (7)`

### Step 37

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `51` -> `52`

**State before/after:**
- `unique_0x58500`: `0x00 (0)` -> `0x01 (1)`

### Step 38

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `52` -> `53`

**State before/after:**
- `PF`: `0x01 (1)` -> `0x00 (0)`

### Step 39

**P-Code:** `INT_EQUAL OF:1B, SF:1B -> unique_0x25c00:1B`

**Program counter:** `53` -> `54`

**State before/after:**
- `unique_0x25c00`: `<unmapped>` -> `0x00 (0)`

### Step 40

**P-Code:** `CBRANCH ram[0x14000125d]:8B, unique_0x25c00:1B`

**Control flow:** fall-through (target 102)
**Program counter:** `54` -> `55`

**State before/after:**
- No register or unique-byte changes.

### Step 41

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x0`
**Program counter:** `55` -> `56`

**State before/after:**
- No register or unique-byte changes.

### Step 42

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RAX:8B`

**Program counter:** `56` -> `57`

**State before/after:**
- `EAX`: `0x03 (3)` -> `0x00 (0)`
- `RAX`: `0x03 (3)` -> `0x00 (0)`

### Step 43

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x0`
**Program counter:** `57` -> `58`

**State before/after:**
- No register or unique-byte changes.

### Step 44

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RCX:8B`

**Program counter:** `58` -> `59`

**State before/after:**
- `RCX`: `0x10000000 (268435456)` -> `0x00 (0)`

### Step 45

**P-Code:** `INT_ADD 0x20:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `59` -> `60`

**State before/after:**
- `unique_0x9d00`: `0x200018 (2097176)` -> `0x200008 (2097160)`

### Step 46

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200008/8B = 0x10000000`
**Program counter:** `60` -> `61`

**State before/after:**
- `unique_0x23e00`: `<unmapped>` -> `0x10000000 (268435456)`

### Step 47

**P-Code:** `COPY unique_0x23e00:8B -> RDX:8B`

**Program counter:** `61` -> `62`

**State before/after:**
- `RDX`: `0x10000100 (268435712)` -> `0x10000000 (268435456)`

### Step 48

**P-Code:** `INT_ADD 0x28:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `62` -> `63`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x200010 (2097168)`

### Step 49

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200010/8B = 0x10000100`
**Program counter:** `63` -> `64`

**State before/after:**
- `unique_0x23e00`: `0x10000000 (268435456)` -> `0x10000100 (268435712)`

### Step 50

**P-Code:** `COPY unique_0x23e00:8B -> R8:8B`

**Program counter:** `64` -> `65`

**State before/after:**
- `R8`: `0x03 (3)` -> `0x10000100 (268435712)`

### Step 51

**P-Code:** `INT_MULT RAX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `65` -> `66`

**State before/after:**
- `unique_0x9300`: `<unmapped>` -> `0x00 (0)`

### Step 52

**P-Code:** `INT_ADD RDX:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `66` -> `67`

**State before/after:**
- `unique_0x9500`: `<unmapped>` -> `0x10000000 (268435456)`

### Step 53

**P-Code:** `LOAD 0x1b1:4B, unique_0x9500:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x10000000/4B = 0x1`
**Program counter:** `67` -> `68`

**State before/after:**
- `unique_0x23d00`: `0x00 (0)` -> `0x01 (1)`

### Step 54

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `68` -> `69`

**State before/after:**
- `EAX`: `0x00 (0)` -> `0x01 (1)`
- `RAX`: `0x00 (0)` -> `0x01 (1)`

### Step 55

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `69` -> `70`

**State before/after:**
- No register or unique-byte changes.

### Step 56

**P-Code:** `INT_MULT RCX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `70` -> `71`

**State before/after:**
- No register or unique-byte changes.

### Step 57

**P-Code:** `INT_ADD R8:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `71` -> `72`

**State before/after:**
- `unique_0x9500`: `0x10000000 (268435456)` -> `0x10000100 (268435712)`

### Step 58

**P-Code:** `INT_SEXT EAX:4B -> unique_0x92500:8B`

**Program counter:** `72` -> `73`

**State before/after:**
- `unique_0x92500`: `<unmapped>` -> `0x01 (1)`

### Step 59

**P-Code:** `LOAD 0x1b1:4B, unique_0x9500:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x10000100/4B = 0x4`
**Program counter:** `73` -> `74`

**State before/after:**
- `unique_0x23d00`: `0x01 (1)` -> `0x04 (4)`

### Step 60

**P-Code:** `INT_SEXT unique_0x23d00:4B -> unique_0x92600:8B`

**Program counter:** `74` -> `75`

**State before/after:**
- `unique_0x92600`: `<unmapped>` -> `0x04 (4)`

### Step 61

**P-Code:** `INT_MULT unique_0x92500:8B, unique_0x92600:8B -> unique_0x92800:8B`

**Program counter:** `75` -> `76`

**State before/after:**
- `unique_0x92800`: `<unmapped>` -> `0x04 (4)`

### Step 62

**P-Code:** `SUBPIECE unique_0x92800:8B, 0x0:4B -> EAX:4B`

**Program counter:** `76` -> `77`

**State before/after:**
- `EAX`: `0x01 (1)` -> `0x04 (4)`
- `RAX`: `0x01 (1)` -> `0x04 (4)`

### Step 63

**P-Code:** `SUBPIECE unique_0x92800:8B, 0x4:4B -> unique_0x92b00:4B`

**Program counter:** `77` -> `78`

**State before/after:**
- `unique_0x92b00`: `<unmapped>` -> `0x00 (0)`

### Step 64

**P-Code:** `INT_SEXT EAX:4B -> unique_0x5a200:8B`

**Program counter:** `78` -> `79`

**State before/after:**
- `unique_0x5a200`: `<unmapped>` -> `0x04 (4)`

### Step 65

**P-Code:** `INT_NOTEQUAL unique_0x5a200:8B, unique_0x92800:8B -> CF:1B`

**Program counter:** `79` -> `80`

**State before/after:**
- `CF`: `0x01 (1)` -> `0x00 (0)`

### Step 66

**P-Code:** `COPY CF:1B -> OF:1B`

**Program counter:** `80` -> `81`

**State before/after:**
- No register or unique-byte changes.

### Step 67

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `81` -> `82`

**State before/after:**
- No register or unique-byte changes.

### Step 68

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `82` -> `83`

**State before/after:**
- `unique_0x9d00`: `0x200010 (2097168)` -> `0x1fffec (2097132)`

### Step 69

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffec/4B = 0x0`
**Program counter:** `83` -> `84`

**State before/after:**
- `unique_0x23d00`: `0x04 (4)` -> `0x00 (0)`

### Step 70

**P-Code:** `COPY unique_0x23d00:4B -> ECX:4B`

**Program counter:** `84` -> `85`

**State before/after:**
- `ECX`: `<unmapped>` -> `0x00 (0)`

### Step 71

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `85` -> `86`

**State before/after:**
- No register or unique-byte changes.

### Step 72

**P-Code:** `INT_CARRY ECX:4B, EAX:4B -> CF:1B`

**Program counter:** `86` -> `87`

**State before/after:**
- No register or unique-byte changes.

### Step 73

**P-Code:** `INT_SCARRY ECX:4B, EAX:4B -> OF:1B`

**Program counter:** `87` -> `88`

**State before/after:**
- No register or unique-byte changes.

### Step 74

**P-Code:** `INT_ADD ECX:4B, EAX:4B -> ECX:4B`

**Program counter:** `88` -> `89`

**State before/after:**
- `ECX`: `0x00 (0)` -> `0x04 (4)`
- `RCX`: `0x00 (0)` -> `0x04 (4)`

### Step 75

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `89` -> `90`

**State before/after:**
- No register or unique-byte changes.

### Step 76

**P-Code:** `INT_SLESS ECX:4B, 0x0:4B -> SF:1B`

**Program counter:** `90` -> `91`

**State before/after:**
- `SF`: `0x01 (1)` -> `0x00 (0)`

### Step 77

**P-Code:** `INT_EQUAL ECX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `91` -> `92`

**State before/after:**
- No register or unique-byte changes.

### Step 78

**P-Code:** `INT_AND ECX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `92` -> `93`

**State before/after:**
- `unique_0x58300`: `0xfd (253)` -> `0x04 (4)`
- `unique_0x58300`: `0xfd (253)` -> `0x04 (4)`

### Step 79

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `93` -> `94`

**State before/after:**
- `unique_0x58400`: `0x07 (7)` -> `0x01 (1)`

### Step 80

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `94` -> `95`

**State before/after:**
- No register or unique-byte changes.

### Step 81

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `95` -> `96`

**State before/after:**
- No register or unique-byte changes.

### Step 82

**P-Code:** `COPY ECX:4B -> EAX:4B`

**Program counter:** `96` -> `97`

**State before/after:**
- No register or unique-byte changes.

### Step 83

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `97` -> `98`

**State before/after:**
- No register or unique-byte changes.

### Step 84

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `98` -> `99`

**State before/after:**
- No register or unique-byte changes.

### Step 85

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `99` -> `100`

**State before/after:**
- `unique_0xd400`: `0x00 (0)` -> `0x04 (4)`

### Step 86

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffec/4B = 0x4`
**Program counter:** `100` -> `101`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffec`: `0x00 (0)` -> `0x04 (4)`

### Step 87

**P-Code:** `BRANCH ram[0x140001224]:8B`

**Control flow:** branch taken (target 24)
**Program counter:** `101` -> `24`

**State before/after:**
- No register or unique-byte changes.

### Step 88

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x0`
**Program counter:** `24` -> `25`

**State before/after:**
- No register or unique-byte changes.

### Step 89

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `25` -> `26`

**State before/after:**
- `EAX`: `0x04 (4)` -> `0x00 (0)`
- `RAX`: `0x04 (4)` -> `0x00 (0)`

### Step 90

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `26` -> `27`

**State before/after:**
- No register or unique-byte changes.

### Step 91

**P-Code:** `INT_SCARRY EAX:4B, 0x1:4B -> OF:1B`

**Program counter:** `27` -> `28`

**State before/after:**
- No register or unique-byte changes.

### Step 92

**P-Code:** `INT_ADD EAX:4B, 0x1:4B -> EAX:4B`

**Program counter:** `28` -> `29`

**State before/after:**
- `EAX`: `0x00 (0)` -> `0x01 (1)`
- `RAX`: `0x00 (0)` -> `0x01 (1)`

### Step 93

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `29` -> `30`

**State before/after:**
- No register or unique-byte changes.

### Step 94

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `30` -> `31`

**State before/after:**
- No register or unique-byte changes.

### Step 95

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `31` -> `32`

**State before/after:**
- No register or unique-byte changes.

### Step 96

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `32` -> `33`

**State before/after:**
- `unique_0x58300`: `0x04 (4)` -> `0x01 (1)`
- `unique_0x58300`: `0x04 (4)` -> `0x01 (1)`

### Step 97

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `33` -> `34`

**State before/after:**
- No register or unique-byte changes.

### Step 98

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `34` -> `35`

**State before/after:**
- No register or unique-byte changes.

### Step 99

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `35` -> `36`

**State before/after:**
- No register or unique-byte changes.

### Step 100

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `36` -> `37`

**State before/after:**
- `unique_0xd400`: `0x04 (4)` -> `0x01 (1)`

### Step 101

**P-Code:** `STORE 0x1b1:4B, RSP:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffe8/4B = 0x1`
**Program counter:** `37` -> `38`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffe8`: `0x00 (0)` -> `0x01 (1)`

### Step 102

**P-Code:** `INT_ADD 0x30:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `38` -> `39`

**State before/after:**
- `unique_0x9d00`: `0x1fffec (2097132)` -> `0x200018 (2097176)`

### Step 103

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200018/4B = 0x3`
**Program counter:** `39` -> `40`

**State before/after:**
- `unique_0x23d00`: `0x00 (0)` -> `0x03 (3)`

### Step 104

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `40` -> `41`

**State before/after:**
- `EAX`: `0x01 (1)` -> `0x03 (3)`
- `RAX`: `0x01 (1)` -> `0x03 (3)`

### Step 105

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `41` -> `42`

**State before/after:**
- No register or unique-byte changes.

### Step 106

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x1`
**Program counter:** `42` -> `43`

**State before/after:**
- `unique_0x23d00`: `0x03 (3)` -> `0x01 (1)`

### Step 107

**P-Code:** `COPY unique_0x23d00:4B -> unique_0x79700:4B`

**Program counter:** `43` -> `44`

**State before/after:**
- `unique_0x79700`: `0x00 (0)` -> `0x01 (1)`

### Step 108

**P-Code:** `INT_LESS unique_0x79700:4B, EAX:4B -> CF:1B`

**Program counter:** `44` -> `45`

**State before/after:**
- `CF`: `0x00 (0)` -> `0x01 (1)`

### Step 109

**P-Code:** `INT_SBORROW unique_0x79700:4B, EAX:4B -> OF:1B`

**Program counter:** `45` -> `46`

**State before/after:**
- No register or unique-byte changes.

### Step 110

**P-Code:** `INT_SUB unique_0x79700:4B, EAX:4B -> unique_0x79900:4B`

**Program counter:** `46` -> `47`

**State before/after:**
- `unique_0x79900`: `0xfffffffd (4294967293)` -> `0xfffffffe (4294967294)`

### Step 111

**P-Code:** `INT_SLESS unique_0x79900:4B, 0x0:4B -> SF:1B`

**Program counter:** `47` -> `48`

**State before/after:**
- `SF`: `0x00 (0)` -> `0x01 (1)`

### Step 112

**P-Code:** `INT_EQUAL unique_0x79900:4B, 0x0:4B -> ZF:1B`

**Program counter:** `48` -> `49`

**State before/after:**
- No register or unique-byte changes.

### Step 113

**P-Code:** `INT_AND unique_0x79900:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `49` -> `50`

**State before/after:**
- `unique_0x58300`: `0x01 (1)` -> `0xfe (254)`
- `unique_0x58300`: `0x01 (1)` -> `0xfe (254)`

### Step 114

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `50` -> `51`

**State before/after:**
- `unique_0x58400`: `0x01 (1)` -> `0x07 (7)`

### Step 115

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `51` -> `52`

**State before/after:**
- No register or unique-byte changes.

### Step 116

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `52` -> `53`

**State before/after:**
- No register or unique-byte changes.

### Step 117

**P-Code:** `INT_EQUAL OF:1B, SF:1B -> unique_0x25c00:1B`

**Program counter:** `53` -> `54`

**State before/after:**
- No register or unique-byte changes.

### Step 118

**P-Code:** `CBRANCH ram[0x14000125d]:8B, unique_0x25c00:1B`

**Control flow:** fall-through (target 102)
**Program counter:** `54` -> `55`

**State before/after:**
- No register or unique-byte changes.

### Step 119

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x1`
**Program counter:** `55` -> `56`

**State before/after:**
- No register or unique-byte changes.

### Step 120

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RAX:8B`

**Program counter:** `56` -> `57`

**State before/after:**
- `EAX`: `0x03 (3)` -> `0x01 (1)`
- `RAX`: `0x03 (3)` -> `0x01 (1)`

### Step 121

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x1`
**Program counter:** `57` -> `58`

**State before/after:**
- No register or unique-byte changes.

### Step 122

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RCX:8B`

**Program counter:** `58` -> `59`

**State before/after:**
- `ECX`: `0x04 (4)` -> `0x01 (1)`
- `RCX`: `0x04 (4)` -> `0x01 (1)`

### Step 123

**P-Code:** `INT_ADD 0x20:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `59` -> `60`

**State before/after:**
- `unique_0x9d00`: `0x200018 (2097176)` -> `0x200008 (2097160)`

### Step 124

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200008/8B = 0x10000000`
**Program counter:** `60` -> `61`

**State before/after:**
- `unique_0x23e00`: `0x10000100 (268435712)` -> `0x10000000 (268435456)`

### Step 125

**P-Code:** `COPY unique_0x23e00:8B -> RDX:8B`

**Program counter:** `61` -> `62`

**State before/after:**
- No register or unique-byte changes.

### Step 126

**P-Code:** `INT_ADD 0x28:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `62` -> `63`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x200010 (2097168)`

### Step 127

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200010/8B = 0x10000100`
**Program counter:** `63` -> `64`

**State before/after:**
- `unique_0x23e00`: `0x10000000 (268435456)` -> `0x10000100 (268435712)`

### Step 128

**P-Code:** `COPY unique_0x23e00:8B -> R8:8B`

**Program counter:** `64` -> `65`

**State before/after:**
- No register or unique-byte changes.

### Step 129

**P-Code:** `INT_MULT RAX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `65` -> `66`

**State before/after:**
- `unique_0x9300`: `0x00 (0)` -> `0x04 (4)`

### Step 130

**P-Code:** `INT_ADD RDX:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `66` -> `67`

**State before/after:**
- `unique_0x9500`: `0x10000100 (268435712)` -> `0x10000004 (268435460)`

### Step 131

**P-Code:** `LOAD 0x1b1:4B, unique_0x9500:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x10000004/4B = 0x2`
**Program counter:** `67` -> `68`

**State before/after:**
- `unique_0x23d00`: `0x01 (1)` -> `0x02 (2)`

### Step 132

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `68` -> `69`

**State before/after:**
- `EAX`: `0x01 (1)` -> `0x02 (2)`
- `RAX`: `0x01 (1)` -> `0x02 (2)`

### Step 133

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `69` -> `70`

**State before/after:**
- No register or unique-byte changes.

### Step 134

**P-Code:** `INT_MULT RCX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `70` -> `71`

**State before/after:**
- No register or unique-byte changes.

### Step 135

**P-Code:** `INT_ADD R8:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `71` -> `72`

**State before/after:**
- `unique_0x9500`: `0x10000004 (268435460)` -> `0x10000104 (268435716)`

### Step 136

**P-Code:** `INT_SEXT EAX:4B -> unique_0x92500:8B`

**Program counter:** `72` -> `73`

**State before/after:**
- `unique_0x92500`: `0x01 (1)` -> `0x02 (2)`

### Step 137

**P-Code:** `LOAD 0x1b1:4B, unique_0x9500:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x10000104/4B = 0x5`
**Program counter:** `73` -> `74`

**State before/after:**
- `unique_0x23d00`: `0x02 (2)` -> `0x05 (5)`

### Step 138

**P-Code:** `INT_SEXT unique_0x23d00:4B -> unique_0x92600:8B`

**Program counter:** `74` -> `75`

**State before/after:**
- `unique_0x92600`: `0x04 (4)` -> `0x05 (5)`

### Step 139

**P-Code:** `INT_MULT unique_0x92500:8B, unique_0x92600:8B -> unique_0x92800:8B`

**Program counter:** `75` -> `76`

**State before/after:**
- `unique_0x92800`: `0x04 (4)` -> `0x0a (10)`

### Step 140

**P-Code:** `SUBPIECE unique_0x92800:8B, 0x0:4B -> EAX:4B`

**Program counter:** `76` -> `77`

**State before/after:**
- `EAX`: `0x02 (2)` -> `0x0a (10)`
- `RAX`: `0x02 (2)` -> `0x0a (10)`

### Step 141

**P-Code:** `SUBPIECE unique_0x92800:8B, 0x4:4B -> unique_0x92b00:4B`

**Program counter:** `77` -> `78`

**State before/after:**
- No register or unique-byte changes.

### Step 142

**P-Code:** `INT_SEXT EAX:4B -> unique_0x5a200:8B`

**Program counter:** `78` -> `79`

**State before/after:**
- `unique_0x5a200`: `0x04 (4)` -> `0x0a (10)`

### Step 143

**P-Code:** `INT_NOTEQUAL unique_0x5a200:8B, unique_0x92800:8B -> CF:1B`

**Program counter:** `79` -> `80`

**State before/after:**
- `CF`: `0x01 (1)` -> `0x00 (0)`

### Step 144

**P-Code:** `COPY CF:1B -> OF:1B`

**Program counter:** `80` -> `81`

**State before/after:**
- No register or unique-byte changes.

### Step 145

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `81` -> `82`

**State before/after:**
- No register or unique-byte changes.

### Step 146

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `82` -> `83`

**State before/after:**
- `unique_0x9d00`: `0x200010 (2097168)` -> `0x1fffec (2097132)`

### Step 147

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffec/4B = 0x4`
**Program counter:** `83` -> `84`

**State before/after:**
- `unique_0x23d00`: `0x05 (5)` -> `0x04 (4)`

### Step 148

**P-Code:** `COPY unique_0x23d00:4B -> ECX:4B`

**Program counter:** `84` -> `85`

**State before/after:**
- `ECX`: `0x01 (1)` -> `0x04 (4)`
- `RCX`: `0x01 (1)` -> `0x04 (4)`

### Step 149

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `85` -> `86`

**State before/after:**
- No register or unique-byte changes.

### Step 150

**P-Code:** `INT_CARRY ECX:4B, EAX:4B -> CF:1B`

**Program counter:** `86` -> `87`

**State before/after:**
- No register or unique-byte changes.

### Step 151

**P-Code:** `INT_SCARRY ECX:4B, EAX:4B -> OF:1B`

**Program counter:** `87` -> `88`

**State before/after:**
- No register or unique-byte changes.

### Step 152

**P-Code:** `INT_ADD ECX:4B, EAX:4B -> ECX:4B`

**Program counter:** `88` -> `89`

**State before/after:**
- `ECX`: `0x04 (4)` -> `0x0e (14)`
- `RCX`: `0x04 (4)` -> `0x0e (14)`

### Step 153

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `89` -> `90`

**State before/after:**
- No register or unique-byte changes.

### Step 154

**P-Code:** `INT_SLESS ECX:4B, 0x0:4B -> SF:1B`

**Program counter:** `90` -> `91`

**State before/after:**
- `SF`: `0x01 (1)` -> `0x00 (0)`

### Step 155

**P-Code:** `INT_EQUAL ECX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `91` -> `92`

**State before/after:**
- No register or unique-byte changes.

### Step 156

**P-Code:** `INT_AND ECX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `92` -> `93`

**State before/after:**
- `unique_0x58300`: `0xfe (254)` -> `0x0e (14)`
- `unique_0x58300`: `0xfe (254)` -> `0x0e (14)`

### Step 157

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `93` -> `94`

**State before/after:**
- `unique_0x58400`: `0x07 (7)` -> `0x03 (3)`

### Step 158

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `94` -> `95`

**State before/after:**
- No register or unique-byte changes.

### Step 159

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `95` -> `96`

**State before/after:**
- No register or unique-byte changes.

### Step 160

**P-Code:** `COPY ECX:4B -> EAX:4B`

**Program counter:** `96` -> `97`

**State before/after:**
- `EAX`: `0x0a (10)` -> `0x0e (14)`
- `RAX`: `0x0a (10)` -> `0x0e (14)`

### Step 161

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `97` -> `98`

**State before/after:**
- No register or unique-byte changes.

### Step 162

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `98` -> `99`

**State before/after:**
- No register or unique-byte changes.

### Step 163

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `99` -> `100`

**State before/after:**
- `unique_0xd400`: `0x01 (1)` -> `0x0e (14)`

### Step 164

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffec/4B = 0xe`
**Program counter:** `100` -> `101`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffec`: `0x04 (4)` -> `0x0e (14)`

### Step 165

**P-Code:** `BRANCH ram[0x140001224]:8B`

**Control flow:** branch taken (target 24)
**Program counter:** `101` -> `24`

**State before/after:**
- No register or unique-byte changes.

### Step 166

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x1`
**Program counter:** `24` -> `25`

**State before/after:**
- `unique_0x23d00`: `0x04 (4)` -> `0x01 (1)`

### Step 167

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `25` -> `26`

**State before/after:**
- `EAX`: `0x0e (14)` -> `0x01 (1)`
- `RAX`: `0x0e (14)` -> `0x01 (1)`

### Step 168

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `26` -> `27`

**State before/after:**
- No register or unique-byte changes.

### Step 169

**P-Code:** `INT_SCARRY EAX:4B, 0x1:4B -> OF:1B`

**Program counter:** `27` -> `28`

**State before/after:**
- No register or unique-byte changes.

### Step 170

**P-Code:** `INT_ADD EAX:4B, 0x1:4B -> EAX:4B`

**Program counter:** `28` -> `29`

**State before/after:**
- `EAX`: `0x01 (1)` -> `0x02 (2)`
- `RAX`: `0x01 (1)` -> `0x02 (2)`

### Step 171

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `29` -> `30`

**State before/after:**
- No register or unique-byte changes.

### Step 172

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `30` -> `31`

**State before/after:**
- No register or unique-byte changes.

### Step 173

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `31` -> `32`

**State before/after:**
- No register or unique-byte changes.

### Step 174

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `32` -> `33`

**State before/after:**
- `unique_0x58300`: `0x0e (14)` -> `0x02 (2)`
- `unique_0x58300`: `0x0e (14)` -> `0x02 (2)`

### Step 175

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `33` -> `34`

**State before/after:**
- `unique_0x58400`: `0x03 (3)` -> `0x01 (1)`

### Step 176

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `34` -> `35`

**State before/after:**
- No register or unique-byte changes.

### Step 177

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `35` -> `36`

**State before/after:**
- No register or unique-byte changes.

### Step 178

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `36` -> `37`

**State before/after:**
- `unique_0xd400`: `0x0e (14)` -> `0x02 (2)`

### Step 179

**P-Code:** `STORE 0x1b1:4B, RSP:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffe8/4B = 0x2`
**Program counter:** `37` -> `38`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffe8`: `0x01 (1)` -> `0x02 (2)`

### Step 180

**P-Code:** `INT_ADD 0x30:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `38` -> `39`

**State before/after:**
- `unique_0x9d00`: `0x1fffec (2097132)` -> `0x200018 (2097176)`

### Step 181

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200018/4B = 0x3`
**Program counter:** `39` -> `40`

**State before/after:**
- `unique_0x23d00`: `0x01 (1)` -> `0x03 (3)`

### Step 182

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `40` -> `41`

**State before/after:**
- `EAX`: `0x02 (2)` -> `0x03 (3)`
- `RAX`: `0x02 (2)` -> `0x03 (3)`

### Step 183

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `41` -> `42`

**State before/after:**
- No register or unique-byte changes.

### Step 184

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x2`
**Program counter:** `42` -> `43`

**State before/after:**
- `unique_0x23d00`: `0x03 (3)` -> `0x02 (2)`

### Step 185

**P-Code:** `COPY unique_0x23d00:4B -> unique_0x79700:4B`

**Program counter:** `43` -> `44`

**State before/after:**
- `unique_0x79700`: `0x01 (1)` -> `0x02 (2)`

### Step 186

**P-Code:** `INT_LESS unique_0x79700:4B, EAX:4B -> CF:1B`

**Program counter:** `44` -> `45`

**State before/after:**
- `CF`: `0x00 (0)` -> `0x01 (1)`

### Step 187

**P-Code:** `INT_SBORROW unique_0x79700:4B, EAX:4B -> OF:1B`

**Program counter:** `45` -> `46`

**State before/after:**
- No register or unique-byte changes.

### Step 188

**P-Code:** `INT_SUB unique_0x79700:4B, EAX:4B -> unique_0x79900:4B`

**Program counter:** `46` -> `47`

**State before/after:**
- `unique_0x79900`: `0xfffffffe (4294967294)` -> `0xffffffff (4294967295)`

### Step 189

**P-Code:** `INT_SLESS unique_0x79900:4B, 0x0:4B -> SF:1B`

**Program counter:** `47` -> `48`

**State before/after:**
- `SF`: `0x00 (0)` -> `0x01 (1)`

### Step 190

**P-Code:** `INT_EQUAL unique_0x79900:4B, 0x0:4B -> ZF:1B`

**Program counter:** `48` -> `49`

**State before/after:**
- No register or unique-byte changes.

### Step 191

**P-Code:** `INT_AND unique_0x79900:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `49` -> `50`

**State before/after:**
- `unique_0x58300`: `0x02 (2)` -> `0xff (255)`
- `unique_0x58300`: `0x02 (2)` -> `0xff (255)`

### Step 192

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `50` -> `51`

**State before/after:**
- `unique_0x58400`: `0x01 (1)` -> `0x08 (8)`

### Step 193

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `51` -> `52`

**State before/after:**
- `unique_0x58500`: `0x01 (1)` -> `0x00 (0)`

### Step 194

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `52` -> `53`

**State before/after:**
- `PF`: `0x00 (0)` -> `0x01 (1)`

### Step 195

**P-Code:** `INT_EQUAL OF:1B, SF:1B -> unique_0x25c00:1B`

**Program counter:** `53` -> `54`

**State before/after:**
- No register or unique-byte changes.

### Step 196

**P-Code:** `CBRANCH ram[0x14000125d]:8B, unique_0x25c00:1B`

**Control flow:** fall-through (target 102)
**Program counter:** `54` -> `55`

**State before/after:**
- No register or unique-byte changes.

### Step 197

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x2`
**Program counter:** `55` -> `56`

**State before/after:**
- No register or unique-byte changes.

### Step 198

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RAX:8B`

**Program counter:** `56` -> `57`

**State before/after:**
- `EAX`: `0x03 (3)` -> `0x02 (2)`
- `RAX`: `0x03 (3)` -> `0x02 (2)`

### Step 199

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x2`
**Program counter:** `57` -> `58`

**State before/after:**
- No register or unique-byte changes.

### Step 200

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RCX:8B`

**Program counter:** `58` -> `59`

**State before/after:**
- `ECX`: `0x0e (14)` -> `0x02 (2)`
- `RCX`: `0x0e (14)` -> `0x02 (2)`

### Step 201

**P-Code:** `INT_ADD 0x20:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `59` -> `60`

**State before/after:**
- `unique_0x9d00`: `0x200018 (2097176)` -> `0x200008 (2097160)`

### Step 202

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200008/8B = 0x10000000`
**Program counter:** `60` -> `61`

**State before/after:**
- `unique_0x23e00`: `0x10000100 (268435712)` -> `0x10000000 (268435456)`

### Step 203

**P-Code:** `COPY unique_0x23e00:8B -> RDX:8B`

**Program counter:** `61` -> `62`

**State before/after:**
- No register or unique-byte changes.

### Step 204

**P-Code:** `INT_ADD 0x28:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `62` -> `63`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x200010 (2097168)`

### Step 205

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200010/8B = 0x10000100`
**Program counter:** `63` -> `64`

**State before/after:**
- `unique_0x23e00`: `0x10000000 (268435456)` -> `0x10000100 (268435712)`

### Step 206

**P-Code:** `COPY unique_0x23e00:8B -> R8:8B`

**Program counter:** `64` -> `65`

**State before/after:**
- No register or unique-byte changes.

### Step 207

**P-Code:** `INT_MULT RAX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `65` -> `66`

**State before/after:**
- `unique_0x9300`: `0x04 (4)` -> `0x08 (8)`

### Step 208

**P-Code:** `INT_ADD RDX:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `66` -> `67`

**State before/after:**
- `unique_0x9500`: `0x10000104 (268435716)` -> `0x10000008 (268435464)`

### Step 209

**P-Code:** `LOAD 0x1b1:4B, unique_0x9500:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x10000008/4B = 0x3`
**Program counter:** `67` -> `68`

**State before/after:**
- `unique_0x23d00`: `0x02 (2)` -> `0x03 (3)`

### Step 210

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `68` -> `69`

**State before/after:**
- `EAX`: `0x02 (2)` -> `0x03 (3)`
- `RAX`: `0x02 (2)` -> `0x03 (3)`

### Step 211

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `69` -> `70`

**State before/after:**
- No register or unique-byte changes.

### Step 212

**P-Code:** `INT_MULT RCX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `70` -> `71`

**State before/after:**
- No register or unique-byte changes.

### Step 213

**P-Code:** `INT_ADD R8:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `71` -> `72`

**State before/after:**
- `unique_0x9500`: `0x10000008 (268435464)` -> `0x10000108 (268435720)`

### Step 214

**P-Code:** `INT_SEXT EAX:4B -> unique_0x92500:8B`

**Program counter:** `72` -> `73`

**State before/after:**
- `unique_0x92500`: `0x02 (2)` -> `0x03 (3)`

### Step 215

**P-Code:** `LOAD 0x1b1:4B, unique_0x9500:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x10000108/4B = 0x6`
**Program counter:** `73` -> `74`

**State before/after:**
- `unique_0x23d00`: `0x03 (3)` -> `0x06 (6)`

### Step 216

**P-Code:** `INT_SEXT unique_0x23d00:4B -> unique_0x92600:8B`

**Program counter:** `74` -> `75`

**State before/after:**
- `unique_0x92600`: `0x05 (5)` -> `0x06 (6)`

### Step 217

**P-Code:** `INT_MULT unique_0x92500:8B, unique_0x92600:8B -> unique_0x92800:8B`

**Program counter:** `75` -> `76`

**State before/after:**
- `unique_0x92800`: `0x0a (10)` -> `0x12 (18)`

### Step 218

**P-Code:** `SUBPIECE unique_0x92800:8B, 0x0:4B -> EAX:4B`

**Program counter:** `76` -> `77`

**State before/after:**
- `EAX`: `0x03 (3)` -> `0x12 (18)`
- `RAX`: `0x03 (3)` -> `0x12 (18)`

### Step 219

**P-Code:** `SUBPIECE unique_0x92800:8B, 0x4:4B -> unique_0x92b00:4B`

**Program counter:** `77` -> `78`

**State before/after:**
- No register or unique-byte changes.

### Step 220

**P-Code:** `INT_SEXT EAX:4B -> unique_0x5a200:8B`

**Program counter:** `78` -> `79`

**State before/after:**
- `unique_0x5a200`: `0x0a (10)` -> `0x12 (18)`

### Step 221

**P-Code:** `INT_NOTEQUAL unique_0x5a200:8B, unique_0x92800:8B -> CF:1B`

**Program counter:** `79` -> `80`

**State before/after:**
- `CF`: `0x01 (1)` -> `0x00 (0)`

### Step 222

**P-Code:** `COPY CF:1B -> OF:1B`

**Program counter:** `80` -> `81`

**State before/after:**
- No register or unique-byte changes.

### Step 223

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `81` -> `82`

**State before/after:**
- No register or unique-byte changes.

### Step 224

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `82` -> `83`

**State before/after:**
- `unique_0x9d00`: `0x200010 (2097168)` -> `0x1fffec (2097132)`

### Step 225

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffec/4B = 0xe`
**Program counter:** `83` -> `84`

**State before/after:**
- `unique_0x23d00`: `0x06 (6)` -> `0x0e (14)`

### Step 226

**P-Code:** `COPY unique_0x23d00:4B -> ECX:4B`

**Program counter:** `84` -> `85`

**State before/after:**
- `ECX`: `0x02 (2)` -> `0x0e (14)`
- `RCX`: `0x02 (2)` -> `0x0e (14)`

### Step 227

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `85` -> `86`

**State before/after:**
- No register or unique-byte changes.

### Step 228

**P-Code:** `INT_CARRY ECX:4B, EAX:4B -> CF:1B`

**Program counter:** `86` -> `87`

**State before/after:**
- No register or unique-byte changes.

### Step 229

**P-Code:** `INT_SCARRY ECX:4B, EAX:4B -> OF:1B`

**Program counter:** `87` -> `88`

**State before/after:**
- No register or unique-byte changes.

### Step 230

**P-Code:** `INT_ADD ECX:4B, EAX:4B -> ECX:4B`

**Program counter:** `88` -> `89`

**State before/after:**
- `ECX`: `0x0e (14)` -> `0x20 (32)`
- `RCX`: `0x0e (14)` -> `0x20 (32)`

### Step 231

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `89` -> `90`

**State before/after:**
- No register or unique-byte changes.

### Step 232

**P-Code:** `INT_SLESS ECX:4B, 0x0:4B -> SF:1B`

**Program counter:** `90` -> `91`

**State before/after:**
- `SF`: `0x01 (1)` -> `0x00 (0)`

### Step 233

**P-Code:** `INT_EQUAL ECX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `91` -> `92`

**State before/after:**
- No register or unique-byte changes.

### Step 234

**P-Code:** `INT_AND ECX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `92` -> `93`

**State before/after:**
- `unique_0x58300`: `0xff (255)` -> `0x20 (32)`
- `unique_0x58300`: `0xff (255)` -> `0x20 (32)`

### Step 235

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `93` -> `94`

**State before/after:**
- `unique_0x58400`: `0x08 (8)` -> `0x01 (1)`

### Step 236

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `94` -> `95`

**State before/after:**
- `unique_0x58500`: `0x00 (0)` -> `0x01 (1)`

### Step 237

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `95` -> `96`

**State before/after:**
- `PF`: `0x01 (1)` -> `0x00 (0)`

### Step 238

**P-Code:** `COPY ECX:4B -> EAX:4B`

**Program counter:** `96` -> `97`

**State before/after:**
- `EAX`: `0x12 (18)` -> `0x20 (32)`
- `RAX`: `0x12 (18)` -> `0x20 (32)`

### Step 239

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `97` -> `98`

**State before/after:**
- No register or unique-byte changes.

### Step 240

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `98` -> `99`

**State before/after:**
- No register or unique-byte changes.

### Step 241

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `99` -> `100`

**State before/after:**
- `unique_0xd400`: `0x02 (2)` -> `0x20 (32)`

### Step 242

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffec/4B = 0x20`
**Program counter:** `100` -> `101`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffec`: `0x0e (14)` -> `0x20 (32)`

### Step 243

**P-Code:** `BRANCH ram[0x140001224]:8B`

**Control flow:** branch taken (target 24)
**Program counter:** `101` -> `24`

**State before/after:**
- No register or unique-byte changes.

### Step 244

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x2`
**Program counter:** `24` -> `25`

**State before/after:**
- `unique_0x23d00`: `0x0e (14)` -> `0x02 (2)`

### Step 245

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `25` -> `26`

**State before/after:**
- `EAX`: `0x20 (32)` -> `0x02 (2)`
- `RAX`: `0x20 (32)` -> `0x02 (2)`

### Step 246

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `26` -> `27`

**State before/after:**
- No register or unique-byte changes.

### Step 247

**P-Code:** `INT_SCARRY EAX:4B, 0x1:4B -> OF:1B`

**Program counter:** `27` -> `28`

**State before/after:**
- No register or unique-byte changes.

### Step 248

**P-Code:** `INT_ADD EAX:4B, 0x1:4B -> EAX:4B`

**Program counter:** `28` -> `29`

**State before/after:**
- `EAX`: `0x02 (2)` -> `0x03 (3)`
- `RAX`: `0x02 (2)` -> `0x03 (3)`

### Step 249

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `29` -> `30`

**State before/after:**
- No register or unique-byte changes.

### Step 250

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `30` -> `31`

**State before/after:**
- No register or unique-byte changes.

### Step 251

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `31` -> `32`

**State before/after:**
- No register or unique-byte changes.

### Step 252

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `32` -> `33`

**State before/after:**
- `unique_0x58300`: `0x20 (32)` -> `0x03 (3)`
- `unique_0x58300`: `0x20 (32)` -> `0x03 (3)`

### Step 253

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `33` -> `34`

**State before/after:**
- `unique_0x58400`: `0x01 (1)` -> `0x02 (2)`

### Step 254

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `34` -> `35`

**State before/after:**
- `unique_0x58500`: `0x01 (1)` -> `0x00 (0)`

### Step 255

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `35` -> `36`

**State before/after:**
- `PF`: `0x00 (0)` -> `0x01 (1)`

### Step 256

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `36` -> `37`

**State before/after:**
- `unique_0xd400`: `0x20 (32)` -> `0x03 (3)`

### Step 257

**P-Code:** `STORE 0x1b1:4B, RSP:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffe8/4B = 0x3`
**Program counter:** `37` -> `38`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffe8`: `0x02 (2)` -> `0x03 (3)`

### Step 258

**P-Code:** `INT_ADD 0x30:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `38` -> `39`

**State before/after:**
- `unique_0x9d00`: `0x1fffec (2097132)` -> `0x200018 (2097176)`

### Step 259

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200018/4B = 0x3`
**Program counter:** `39` -> `40`

**State before/after:**
- `unique_0x23d00`: `0x02 (2)` -> `0x03 (3)`

### Step 260

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `40` -> `41`

**State before/after:**
- No register or unique-byte changes.

### Step 261

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `41` -> `42`

**State before/after:**
- No register or unique-byte changes.

### Step 262

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x3`
**Program counter:** `42` -> `43`

**State before/after:**
- No register or unique-byte changes.

### Step 263

**P-Code:** `COPY unique_0x23d00:4B -> unique_0x79700:4B`

**Program counter:** `43` -> `44`

**State before/after:**
- `unique_0x79700`: `0x02 (2)` -> `0x03 (3)`

### Step 264

**P-Code:** `INT_LESS unique_0x79700:4B, EAX:4B -> CF:1B`

**Program counter:** `44` -> `45`

**State before/after:**
- No register or unique-byte changes.

### Step 265

**P-Code:** `INT_SBORROW unique_0x79700:4B, EAX:4B -> OF:1B`

**Program counter:** `45` -> `46`

**State before/after:**
- No register or unique-byte changes.

### Step 266

**P-Code:** `INT_SUB unique_0x79700:4B, EAX:4B -> unique_0x79900:4B`

**Program counter:** `46` -> `47`

**State before/after:**
- `unique_0x79900`: `0xffffffff (4294967295)` -> `0x00 (0)`

### Step 267

**P-Code:** `INT_SLESS unique_0x79900:4B, 0x0:4B -> SF:1B`

**Program counter:** `47` -> `48`

**State before/after:**
- No register or unique-byte changes.

### Step 268

**P-Code:** `INT_EQUAL unique_0x79900:4B, 0x0:4B -> ZF:1B`

**Program counter:** `48` -> `49`

**State before/after:**
- `ZF`: `0x00 (0)` -> `0x01 (1)`

### Step 269

**P-Code:** `INT_AND unique_0x79900:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `49` -> `50`

**State before/after:**
- `unique_0x58300`: `0x03 (3)` -> `0x00 (0)`
- `unique_0x58300`: `0x03 (3)` -> `0x00 (0)`

### Step 270

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `50` -> `51`

**State before/after:**
- `unique_0x58400`: `0x02 (2)` -> `0x00 (0)`

### Step 271

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `51` -> `52`

**State before/after:**
- No register or unique-byte changes.

### Step 272

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `52` -> `53`

**State before/after:**
- No register or unique-byte changes.

### Step 273

**P-Code:** `INT_EQUAL OF:1B, SF:1B -> unique_0x25c00:1B`

**Program counter:** `53` -> `54`

**State before/after:**
- `unique_0x25c00`: `0x00 (0)` -> `0x01 (1)`

### Step 274

**P-Code:** `CBRANCH ram[0x14000125d]:8B, unique_0x25c00:1B`

**Control flow:** branch taken (target 102)
**Program counter:** `54` -> `102`

**State before/after:**
- No register or unique-byte changes.

### Step 275

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `102` -> `103`

**State before/after:**
- `unique_0x9d00`: `0x200018 (2097176)` -> `0x1fffec (2097132)`

### Step 276

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffec/4B = 0x20`
**Program counter:** `103` -> `104`

**State before/after:**
- `unique_0x23d00`: `0x03 (3)` -> `0x20 (32)`

### Step 277

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `104` -> `105`

**State before/after:**
- `EAX`: `0x03 (3)` -> `0x20 (32)`
- `RAX`: `0x03 (3)` -> `0x20 (32)`

### Step 278

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `105` -> `106`

**State before/after:**
- No register or unique-byte changes.

### Step 279

**P-Code:** `INT_CARRY RSP:8B, 0x18:8B -> CF:1B`

**Program counter:** `106` -> `107`

**State before/after:**
- No register or unique-byte changes.

### Step 280

**P-Code:** `INT_SCARRY RSP:8B, 0x18:8B -> OF:1B`

**Program counter:** `107` -> `108`

**State before/after:**
- No register or unique-byte changes.

### Step 281

**P-Code:** `INT_ADD RSP:8B, 0x18:8B -> RSP:8B`

**Program counter:** `108` -> `109`

**State before/after:**
- `RSP`: `0x1fffe8 (2097128)` -> `0x200000 (2097152)`

### Step 282

**P-Code:** `INT_SLESS RSP:8B, 0x0:8B -> SF:1B`

**Program counter:** `109` -> `110`

**State before/after:**
- No register or unique-byte changes.

### Step 283

**P-Code:** `INT_EQUAL RSP:8B, 0x0:8B -> ZF:1B`

**Program counter:** `110` -> `111`

**State before/after:**
- `ZF`: `0x01 (1)` -> `0x00 (0)`

### Step 284

**P-Code:** `INT_AND RSP:8B, 0xff:8B -> unique_0x58300:8B`

**Program counter:** `111` -> `112`

**State before/after:**
- No register or unique-byte changes.

### Step 285

**P-Code:** `POPCOUNT unique_0x58300:8B -> unique_0x58400:1B`

**Program counter:** `112` -> `113`

**State before/after:**
- No register or unique-byte changes.

### Step 286

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `113` -> `114`

**State before/after:**
- No register or unique-byte changes.

### Step 287

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `114` -> `115`

**State before/after:**
- No register or unique-byte changes.

### Step 288

**P-Code:** `LOAD 0x1b1:8B, RSP:8B -> RIP:8B`

**Memory reads:** `ram:0x200000/8B = 0xdeadbeef`
**Program counter:** `115` -> `116`

**State before/after:**
- `RIP`: `<unmapped>` -> `0xdeadbeef (3735928559)`

### Step 289

**P-Code:** `INT_ADD RSP:8B, 0x8:8B -> RSP:8B`

**Program counter:** `116` -> `117`

**State before/after:**
- `RSP`: `0x200000 (2097152)` -> `0x200008 (2097160)`

### Step 290

**P-Code:** `RETURN RIP:8B`

**Control flow:** return
**Program counter:** `117` -> `117`

**State before/after:**
- No register or unique-byte changes.

## Final State

- Program counter: `117`
- Halted: `True`

- `int[3] left` @ `0x10000000` (12 bytes): `1, 2, 3`
- `int[3] right` @ `0x10000100` (12 bytes): `4, 5, 6`

## Validation

**Expected result:**
`32`

**Normal C++ result:**
`32`

**Emulated result:**
`32`

**Status:** `PASS`
