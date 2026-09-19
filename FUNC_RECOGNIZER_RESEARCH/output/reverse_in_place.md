# P-Code Execution Trace: `reverse_in_place`

**Arguments:** `(268435456, 4)`

## Initial State

Sparse register/temporary state is shown only when a byte location is changed.

- Program counter: `0`
- `RCX` = `0x10000000` (268435456)
- `RDX` = `0x4` (4)
- `RSP` = `0x200000` (2097152)
- `int[4] values` @ `0x10000000` (16 bytes): `10, 20, 30, 40`

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
- `unique_0xd400`: `<unmapped>` -> `0x04 (4)`

### Step 2

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x200010/4B = 0x4`
**Program counter:** `2` -> `3`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x200010`: `<unmapped>` -> `0x04 (4)`
- `ram:0x200011`: `<unmapped>` -> `0x00 (0)`
- `ram:0x200012`: `<unmapped>` -> `0x00 (0)`
- `ram:0x200013`: `<unmapped>` -> `0x00 (0)`

### Step 3

**P-Code:** `INT_ADD 0x8:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `3` -> `4`

**State before/after:**
- `unique_0x9d00`: `0x200010 (2097168)` -> `0x200008 (2097160)`

### Step 4

**P-Code:** `COPY RCX:8B -> unique_0xd500:8B`

**Program counter:** `4` -> `5`

**State before/after:**
- `unique_0xd500`: `<unmapped>` -> `0x10000000 (268435456)`

### Step 5

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd500:8B`

**Memory writes:** `ram:0x200008/8B = 0x10000000`
**Program counter:** `5` -> `6`

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

**P-Code:** `COPY 0x0:4B -> unique_0xd400:4B`

**Program counter:** `15` -> `16`

**State before/after:**
- `unique_0xd400`: `0x04 (4)` -> `0x00 (0)`

### Step 16

**P-Code:** `STORE 0x1b1:4B, RSP:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffe8/4B = 0x0`
**Program counter:** `16` -> `17`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffe8`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffe9`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffea`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffeb`: `<unmapped>` -> `0x00 (0)`

### Step 17

**P-Code:** `BRANCH ram[0x14000128e]:8B`

**Control flow:** branch taken (target 32)
**Program counter:** `17` -> `32`

**State before/after:**
- No register or unique-byte changes.

### Step 18

**P-Code:** `INT_ADD 0x28:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `32` -> `33`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x200010 (2097168)`

### Step 19

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200010/4B = 0x4`
**Program counter:** `33` -> `34`

**State before/after:**
- `unique_0x23d00`: `<unmapped>` -> `0x04 (4)`

### Step 20

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `34` -> `35`

**State before/after:**
- `EAX`: `<unmapped>` -> `0x04 (4)`

### Step 21

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `35` -> `36`

**State before/after:**
- `RAX`: `<unmapped>` -> `0x04 (4)`

### Step 22

**P-Code:** `INT_SEXT EAX:4B -> unique_0x76600:8B`

**Program counter:** `36` -> `37`

**State before/after:**
- `unique_0x76600`: `<unmapped>` -> `0x04 (4)`

### Step 23

**P-Code:** `SUBPIECE unique_0x76600:8B, 0x4:4B -> EDX:4B`

**Program counter:** `37` -> `38`

**State before/after:**
- `EDX`: `<unmapped>` -> `0x00 (0)`
- `RDX`: `0x04 (4)` -> `0x00 (0)`

### Step 24

**P-Code:** `INT_ZEXT EDX:4B -> RDX:8B`

**Program counter:** `38` -> `39`

**State before/after:**
- No register or unique-byte changes.

### Step 25

**P-Code:** `INT_LESS EAX:4B, EDX:4B -> CF:1B`

**Program counter:** `39` -> `40`

**State before/after:**
- No register or unique-byte changes.

### Step 26

**P-Code:** `INT_SBORROW EAX:4B, EDX:4B -> OF:1B`

**Program counter:** `40` -> `41`

**State before/after:**
- No register or unique-byte changes.

### Step 27

**P-Code:** `INT_SUB EAX:4B, EDX:4B -> EAX:4B`

**Program counter:** `41` -> `42`

**State before/after:**
- No register or unique-byte changes.

### Step 28

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `42` -> `43`

**State before/after:**
- No register or unique-byte changes.

### Step 29

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `43` -> `44`

**State before/after:**
- No register or unique-byte changes.

### Step 30

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `44` -> `45`

**State before/after:**
- No register or unique-byte changes.

### Step 31

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `45` -> `46`

**State before/after:**
- `unique_0x58300`: `<unmapped>` -> `0x04 (4)`
- `unique_0x58300`: `0xe8 (232)` -> `0x04 (4)`

### Step 32

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `46` -> `47`

**State before/after:**
- `unique_0x58400`: `0x04 (4)` -> `0x01 (1)`

### Step 33

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `47` -> `48`

**State before/after:**
- `unique_0x58500`: `0x00 (0)` -> `0x01 (1)`

### Step 34

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `48` -> `49`

**State before/after:**
- `PF`: `0x01 (1)` -> `0x00 (0)`

### Step 35

**P-Code:** `INT_AND EAX:4B, 0x1:4B -> unique_0xc2700:4B`

**Program counter:** `49` -> `50`

**State before/after:**
- `unique_0xc2700`: `<unmapped>` -> `0x00 (0)`

### Step 36

**P-Code:** `INT_NOTEQUAL unique_0xc2700:4B, 0x0:4B -> CF:1B`

**Program counter:** `50` -> `51`

**State before/after:**
- No register or unique-byte changes.

### Step 37

**P-Code:** `COPY 0x0:1B -> OF:1B`

**Program counter:** `51` -> `52`

**State before/after:**
- No register or unique-byte changes.

### Step 38

**P-Code:** `INT_SRIGHT EAX:4B, 0x1:4B -> EAX:4B`

**Program counter:** `52` -> `53`

**State before/after:**
- `EAX`: `0x04 (4)` -> `0x02 (2)`
- `RAX`: `0x04 (4)` -> `0x02 (2)`

### Step 39

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `53` -> `54`

**State before/after:**
- No register or unique-byte changes.

### Step 40

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `54` -> `55`

**State before/after:**
- No register or unique-byte changes.

### Step 41

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `55` -> `56`

**State before/after:**
- No register or unique-byte changes.

### Step 42

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `56` -> `57`

**State before/after:**
- `unique_0x58300`: `0x04 (4)` -> `0x02 (2)`
- `unique_0x58300`: `0x04 (4)` -> `0x02 (2)`

### Step 43

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `57` -> `58`

**State before/after:**
- No register or unique-byte changes.

### Step 44

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `58` -> `59`

**State before/after:**
- No register or unique-byte changes.

### Step 45

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `59` -> `60`

**State before/after:**
- No register or unique-byte changes.

### Step 46

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x0`
**Program counter:** `60` -> `61`

**State before/after:**
- `unique_0x23d00`: `0x04 (4)` -> `0x00 (0)`

### Step 47

**P-Code:** `COPY unique_0x23d00:4B -> unique_0x79700:4B`

**Program counter:** `61` -> `62`

**State before/after:**
- `unique_0x79700`: `<unmapped>` -> `0x00 (0)`

### Step 48

**P-Code:** `INT_LESS unique_0x79700:4B, EAX:4B -> CF:1B`

**Program counter:** `62` -> `63`

**State before/after:**
- `CF`: `0x00 (0)` -> `0x01 (1)`

### Step 49

**P-Code:** `INT_SBORROW unique_0x79700:4B, EAX:4B -> OF:1B`

**Program counter:** `63` -> `64`

**State before/after:**
- No register or unique-byte changes.

### Step 50

**P-Code:** `INT_SUB unique_0x79700:4B, EAX:4B -> unique_0x79900:4B`

**Program counter:** `64` -> `65`

**State before/after:**
- `unique_0x79900`: `<unmapped>` -> `0xfffffffe (4294967294)`

### Step 51

**P-Code:** `INT_SLESS unique_0x79900:4B, 0x0:4B -> SF:1B`

**Program counter:** `65` -> `66`

**State before/after:**
- `SF`: `0x00 (0)` -> `0x01 (1)`

### Step 52

**P-Code:** `INT_EQUAL unique_0x79900:4B, 0x0:4B -> ZF:1B`

**Program counter:** `66` -> `67`

**State before/after:**
- No register or unique-byte changes.

### Step 53

**P-Code:** `INT_AND unique_0x79900:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `67` -> `68`

**State before/after:**
- `unique_0x58300`: `0x02 (2)` -> `0xfe (254)`
- `unique_0x58300`: `0x02 (2)` -> `0xfe (254)`

### Step 54

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `68` -> `69`

**State before/after:**
- `unique_0x58400`: `0x01 (1)` -> `0x07 (7)`

### Step 55

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `69` -> `70`

**State before/after:**
- No register or unique-byte changes.

### Step 56

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `70` -> `71`

**State before/after:**
- No register or unique-byte changes.

### Step 57

**P-Code:** `INT_EQUAL OF:1B, SF:1B -> unique_0x25c00:1B`

**Program counter:** `71` -> `72`

**State before/after:**
- `unique_0x25c00`: `<unmapped>` -> `0x00 (0)`

### Step 58

**P-Code:** `CBRANCH ram[0x1400012ea]:8B, unique_0x25c00:1B`

**Control flow:** fall-through (target 152)
**Program counter:** `72` -> `73`

**State before/after:**
- No register or unique-byte changes.

### Step 59

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x0`
**Program counter:** `73` -> `74`

**State before/after:**
- No register or unique-byte changes.

### Step 60

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `74` -> `75`

**State before/after:**
- `EAX`: `0x02 (2)` -> `0x00 (0)`
- `RAX`: `0x02 (2)` -> `0x00 (0)`

### Step 61

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `75` -> `76`

**State before/after:**
- No register or unique-byte changes.

### Step 62

**P-Code:** `INT_ADD 0x28:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `76` -> `77`

**State before/after:**
- No register or unique-byte changes.

### Step 63

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200010/4B = 0x4`
**Program counter:** `77` -> `78`

**State before/after:**
- `unique_0x23d00`: `0x00 (0)` -> `0x04 (4)`

### Step 64

**P-Code:** `COPY unique_0x23d00:4B -> ECX:4B`

**Program counter:** `78` -> `79`

**State before/after:**
- `ECX`: `<unmapped>` -> `0x04 (4)`
- `RCX`: `0x10000000 (268435456)` -> `0x04 (4)`

### Step 65

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `79` -> `80`

**State before/after:**
- No register or unique-byte changes.

### Step 66

**P-Code:** `INT_LESS ECX:4B, EAX:4B -> CF:1B`

**Program counter:** `80` -> `81`

**State before/after:**
- `CF`: `0x01 (1)` -> `0x00 (0)`

### Step 67

**P-Code:** `INT_SBORROW ECX:4B, EAX:4B -> OF:1B`

**Program counter:** `81` -> `82`

**State before/after:**
- No register or unique-byte changes.

### Step 68

**P-Code:** `INT_SUB ECX:4B, EAX:4B -> ECX:4B`

**Program counter:** `82` -> `83`

**State before/after:**
- No register or unique-byte changes.

### Step 69

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `83` -> `84`

**State before/after:**
- No register or unique-byte changes.

### Step 70

**P-Code:** `INT_SLESS ECX:4B, 0x0:4B -> SF:1B`

**Program counter:** `84` -> `85`

**State before/after:**
- `SF`: `0x01 (1)` -> `0x00 (0)`

### Step 71

**P-Code:** `INT_EQUAL ECX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `85` -> `86`

**State before/after:**
- No register or unique-byte changes.

### Step 72

**P-Code:** `INT_AND ECX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `86` -> `87`

**State before/after:**
- `unique_0x58300`: `0xfe (254)` -> `0x04 (4)`
- `unique_0x58300`: `0xfe (254)` -> `0x04 (4)`

### Step 73

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `87` -> `88`

**State before/after:**
- `unique_0x58400`: `0x07 (7)` -> `0x01 (1)`

### Step 74

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `88` -> `89`

**State before/after:**
- No register or unique-byte changes.

### Step 75

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `89` -> `90`

**State before/after:**
- No register or unique-byte changes.

### Step 76

**P-Code:** `COPY ECX:4B -> EAX:4B`

**Program counter:** `90` -> `91`

**State before/after:**
- `EAX`: `0x00 (0)` -> `0x04 (4)`
- `RAX`: `0x00 (0)` -> `0x04 (4)`

### Step 77

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `91` -> `92`

**State before/after:**
- No register or unique-byte changes.

### Step 78

**P-Code:** `INT_SBORROW EAX:4B, 0x1:4B -> OF:1B`

**Program counter:** `92` -> `93`

**State before/after:**
- No register or unique-byte changes.

### Step 79

**P-Code:** `INT_SUB EAX:4B, 0x1:4B -> EAX:4B`

**Program counter:** `93` -> `94`

**State before/after:**
- `EAX`: `0x04 (4)` -> `0x03 (3)`
- `RAX`: `0x04 (4)` -> `0x03 (3)`

### Step 80

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `94` -> `95`

**State before/after:**
- No register or unique-byte changes.

### Step 81

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `95` -> `96`

**State before/after:**
- No register or unique-byte changes.

### Step 82

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `96` -> `97`

**State before/after:**
- No register or unique-byte changes.

### Step 83

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `97` -> `98`

**State before/after:**
- `unique_0x58300`: `0x04 (4)` -> `0x03 (3)`
- `unique_0x58300`: `0x04 (4)` -> `0x03 (3)`

### Step 84

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `98` -> `99`

**State before/after:**
- `unique_0x58400`: `0x01 (1)` -> `0x02 (2)`

### Step 85

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `99` -> `100`

**State before/after:**
- `unique_0x58500`: `0x01 (1)` -> `0x00 (0)`

### Step 86

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `100` -> `101`

**State before/after:**
- `PF`: `0x00 (0)` -> `0x01 (1)`

### Step 87

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `101` -> `102`

**State before/after:**
- `unique_0x9d00`: `0x200010 (2097168)` -> `0x1fffec (2097132)`

### Step 88

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `102` -> `103`

**State before/after:**
- `unique_0xd400`: `0x00 (0)` -> `0x03 (3)`

### Step 89

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffec/4B = 0x3`
**Program counter:** `103` -> `104`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffec`: `<unmapped>` -> `0x03 (3)`
- `ram:0x1fffed`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffee`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1fffef`: `<unmapped>` -> `0x00 (0)`

### Step 90

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x0`
**Program counter:** `104` -> `105`

**State before/after:**
- `unique_0x23d00`: `0x04 (4)` -> `0x00 (0)`

### Step 91

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RAX:8B`

**Program counter:** `105` -> `106`

**State before/after:**
- `EAX`: `0x03 (3)` -> `0x00 (0)`
- `RAX`: `0x03 (3)` -> `0x00 (0)`

### Step 92

**P-Code:** `INT_ADD 0x20:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `106` -> `107`

**State before/after:**
- `unique_0x9d00`: `0x1fffec (2097132)` -> `0x200008 (2097160)`

### Step 93

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200008/8B = 0x10000000`
**Program counter:** `107` -> `108`

**State before/after:**
- `unique_0x23e00`: `<unmapped>` -> `0x10000000 (268435456)`

### Step 94

**P-Code:** `COPY unique_0x23e00:8B -> RCX:8B`

**Program counter:** `108` -> `109`

**State before/after:**
- `ECX`: `0x04 (4)` -> `0x10000000 (268435456)`
- `RCX`: `0x04 (4)` -> `0x10000000 (268435456)`

### Step 95

**P-Code:** `INT_MULT RAX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `109` -> `110`

**State before/after:**
- `unique_0x9300`: `<unmapped>` -> `0x00 (0)`

### Step 96

**P-Code:** `INT_ADD RCX:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `110` -> `111`

**State before/after:**
- `unique_0x9500`: `<unmapped>` -> `0x10000000 (268435456)`

### Step 97

**P-Code:** `LOAD 0x1b1:4B, unique_0x9500:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x10000000/4B = 0xa`
**Program counter:** `111` -> `112`

**State before/after:**
- `unique_0x23d00`: `0x00 (0)` -> `0x0a (10)`

### Step 98

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `112` -> `113`

**State before/after:**
- `EAX`: `0x00 (0)` -> `0x0a (10)`
- `RAX`: `0x00 (0)` -> `0x0a (10)`

### Step 99

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `113` -> `114`

**State before/after:**
- No register or unique-byte changes.

### Step 100

**P-Code:** `INT_ADD 0x8:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `114` -> `115`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x1ffff0 (2097136)`

### Step 101

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `115` -> `116`

**State before/after:**
- `unique_0xd400`: `0x03 (3)` -> `0x0a (10)`

### Step 102

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1ffff0/4B = 0xa`
**Program counter:** `116` -> `117`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1ffff0`: `<unmapped>` -> `0x0a (10)`
- `ram:0x1ffff1`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1ffff2`: `<unmapped>` -> `0x00 (0)`
- `ram:0x1ffff3`: `<unmapped>` -> `0x00 (0)`

### Step 103

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `117` -> `118`

**State before/after:**
- `unique_0x9d00`: `0x1ffff0 (2097136)` -> `0x1fffec (2097132)`

### Step 104

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffec/4B = 0x3`
**Program counter:** `118` -> `119`

**State before/after:**
- `unique_0x23d00`: `0x0a (10)` -> `0x03 (3)`

### Step 105

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RAX:8B`

**Program counter:** `119` -> `120`

**State before/after:**
- `EAX`: `0x0a (10)` -> `0x03 (3)`
- `RAX`: `0x0a (10)` -> `0x03 (3)`

### Step 106

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x0`
**Program counter:** `120` -> `121`

**State before/after:**
- `unique_0x23d00`: `0x03 (3)` -> `0x00 (0)`

### Step 107

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RCX:8B`

**Program counter:** `121` -> `122`

**State before/after:**
- `ECX`: `0x10000000 (268435456)` -> `0x00 (0)`
- `RCX`: `0x10000000 (268435456)` -> `0x00 (0)`

### Step 108

**P-Code:** `INT_ADD 0x20:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `122` -> `123`

**State before/after:**
- `unique_0x9d00`: `0x1fffec (2097132)` -> `0x200008 (2097160)`

### Step 109

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200008/8B = 0x10000000`
**Program counter:** `123` -> `124`

**State before/after:**
- No register or unique-byte changes.

### Step 110

**P-Code:** `COPY unique_0x23e00:8B -> RDX:8B`

**Program counter:** `124` -> `125`

**State before/after:**
- `EDX`: `0x00 (0)` -> `0x10000000 (268435456)`
- `RDX`: `0x00 (0)` -> `0x10000000 (268435456)`

### Step 111

**P-Code:** `INT_ADD 0x20:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `125` -> `126`

**State before/after:**
- No register or unique-byte changes.

### Step 112

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200008/8B = 0x10000000`
**Program counter:** `126` -> `127`

**State before/after:**
- No register or unique-byte changes.

### Step 113

**P-Code:** `COPY unique_0x23e00:8B -> R8:8B`

**Program counter:** `127` -> `128`

**State before/after:**
- `R8`: `<unmapped>` -> `0x10000000 (268435456)`

### Step 114

**P-Code:** `INT_MULT RAX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `128` -> `129`

**State before/after:**
- `unique_0x9300`: `0x00 (0)` -> `0x0c (12)`

### Step 115

**P-Code:** `INT_ADD R8:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `129` -> `130`

**State before/after:**
- `unique_0x9500`: `0x10000000 (268435456)` -> `0x1000000c (268435468)`

### Step 116

**P-Code:** `LOAD 0x1b1:4B, unique_0x9500:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1000000c/4B = 0x28`
**Program counter:** `130` -> `131`

**State before/after:**
- `unique_0x23d00`: `0x00 (0)` -> `0x28 (40)`

### Step 117

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `131` -> `132`

**State before/after:**
- `EAX`: `0x03 (3)` -> `0x28 (40)`
- `RAX`: `0x03 (3)` -> `0x28 (40)`

### Step 118

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `132` -> `133`

**State before/after:**
- No register or unique-byte changes.

### Step 119

**P-Code:** `INT_MULT RCX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `133` -> `134`

**State before/after:**
- `unique_0x9300`: `0x0c (12)` -> `0x00 (0)`

### Step 120

**P-Code:** `INT_ADD RDX:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `134` -> `135`

**State before/after:**
- `unique_0x9500`: `0x1000000c (268435468)` -> `0x10000000 (268435456)`

### Step 121

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `135` -> `136`

**State before/after:**
- `unique_0xd400`: `0x0a (10)` -> `0x28 (40)`

### Step 122

**P-Code:** `STORE 0x1b1:4B, unique_0x9500:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x10000000/4B = 0x28`
**Program counter:** `136` -> `137`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x10000000`: `0x0a (10)` -> `0x28 (40)`
- `int[4] values` @ `0x10000000` (16 bytes): `40, 20, 30, 40`

### Step 123

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `137` -> `138`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x1fffec (2097132)`

### Step 124

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffec/4B = 0x3`
**Program counter:** `138` -> `139`

**State before/after:**
- `unique_0x23d00`: `0x28 (40)` -> `0x03 (3)`

### Step 125

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RAX:8B`

**Program counter:** `139` -> `140`

**State before/after:**
- `EAX`: `0x28 (40)` -> `0x03 (3)`
- `RAX`: `0x28 (40)` -> `0x03 (3)`

### Step 126

**P-Code:** `INT_ADD 0x20:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `140` -> `141`

**State before/after:**
- `unique_0x9d00`: `0x1fffec (2097132)` -> `0x200008 (2097160)`

### Step 127

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200008/8B = 0x10000000`
**Program counter:** `141` -> `142`

**State before/after:**
- No register or unique-byte changes.

### Step 128

**P-Code:** `COPY unique_0x23e00:8B -> RCX:8B`

**Program counter:** `142` -> `143`

**State before/after:**
- `ECX`: `0x00 (0)` -> `0x10000000 (268435456)`
- `RCX`: `0x00 (0)` -> `0x10000000 (268435456)`

### Step 129

**P-Code:** `INT_ADD 0x8:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `143` -> `144`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x1ffff0 (2097136)`

### Step 130

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1ffff0/4B = 0xa`
**Program counter:** `144` -> `145`

**State before/after:**
- `unique_0x23d00`: `0x03 (3)` -> `0x0a (10)`

### Step 131

**P-Code:** `COPY unique_0x23d00:4B -> EDX:4B`

**Program counter:** `145` -> `146`

**State before/after:**
- `EDX`: `0x10000000 (268435456)` -> `0x0a (10)`
- `RDX`: `0x10000000 (268435456)` -> `0x0a (10)`

### Step 132

**P-Code:** `INT_ZEXT EDX:4B -> RDX:8B`

**Program counter:** `146` -> `147`

**State before/after:**
- No register or unique-byte changes.

### Step 133

**P-Code:** `INT_MULT RAX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `147` -> `148`

**State before/after:**
- `unique_0x9300`: `0x00 (0)` -> `0x0c (12)`

### Step 134

**P-Code:** `INT_ADD RCX:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `148` -> `149`

**State before/after:**
- `unique_0x9500`: `0x10000000 (268435456)` -> `0x1000000c (268435468)`

### Step 135

**P-Code:** `COPY EDX:4B -> unique_0xd400:4B`

**Program counter:** `149` -> `150`

**State before/after:**
- `unique_0xd400`: `0x28 (40)` -> `0x0a (10)`

### Step 136

**P-Code:** `STORE 0x1b1:4B, unique_0x9500:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1000000c/4B = 0xa`
**Program counter:** `150` -> `151`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1000000c`: `0x28 (40)` -> `0x0a (10)`
- `int[4] values` @ `0x10000000` (16 bytes): `40, 20, 30, 10`

### Step 137

**P-Code:** `BRANCH ram[0x140001286]:8B`

**Control flow:** branch taken (target 18)
**Program counter:** `151` -> `18`

**State before/after:**
- No register or unique-byte changes.

### Step 138

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x0`
**Program counter:** `18` -> `19`

**State before/after:**
- `unique_0x23d00`: `0x0a (10)` -> `0x00 (0)`

### Step 139

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `19` -> `20`

**State before/after:**
- `EAX`: `0x03 (3)` -> `0x00 (0)`
- `RAX`: `0x03 (3)` -> `0x00 (0)`

### Step 140

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `20` -> `21`

**State before/after:**
- No register or unique-byte changes.

### Step 141

**P-Code:** `INT_SCARRY EAX:4B, 0x1:4B -> OF:1B`

**Program counter:** `21` -> `22`

**State before/after:**
- No register or unique-byte changes.

### Step 142

**P-Code:** `INT_ADD EAX:4B, 0x1:4B -> EAX:4B`

**Program counter:** `22` -> `23`

**State before/after:**
- `EAX`: `0x00 (0)` -> `0x01 (1)`
- `RAX`: `0x00 (0)` -> `0x01 (1)`

### Step 143

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `23` -> `24`

**State before/after:**
- No register or unique-byte changes.

### Step 144

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `24` -> `25`

**State before/after:**
- No register or unique-byte changes.

### Step 145

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `25` -> `26`

**State before/after:**
- No register or unique-byte changes.

### Step 146

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `26` -> `27`

**State before/after:**
- `unique_0x58300`: `0x03 (3)` -> `0x01 (1)`
- `unique_0x58300`: `0x03 (3)` -> `0x01 (1)`

### Step 147

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `27` -> `28`

**State before/after:**
- `unique_0x58400`: `0x02 (2)` -> `0x01 (1)`

### Step 148

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `28` -> `29`

**State before/after:**
- `unique_0x58500`: `0x00 (0)` -> `0x01 (1)`

### Step 149

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `29` -> `30`

**State before/after:**
- `PF`: `0x01 (1)` -> `0x00 (0)`

### Step 150

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `30` -> `31`

**State before/after:**
- `unique_0xd400`: `0x0a (10)` -> `0x01 (1)`

### Step 151

**P-Code:** `STORE 0x1b1:4B, RSP:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffe8/4B = 0x1`
**Program counter:** `31` -> `32`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffe8`: `0x00 (0)` -> `0x01 (1)`

### Step 152

**P-Code:** `INT_ADD 0x28:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `32` -> `33`

**State before/after:**
- `unique_0x9d00`: `0x1ffff0 (2097136)` -> `0x200010 (2097168)`

### Step 153

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200010/4B = 0x4`
**Program counter:** `33` -> `34`

**State before/after:**
- `unique_0x23d00`: `0x00 (0)` -> `0x04 (4)`

### Step 154

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `34` -> `35`

**State before/after:**
- `EAX`: `0x01 (1)` -> `0x04 (4)`
- `RAX`: `0x01 (1)` -> `0x04 (4)`

### Step 155

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `35` -> `36`

**State before/after:**
- No register or unique-byte changes.

### Step 156

**P-Code:** `INT_SEXT EAX:4B -> unique_0x76600:8B`

**Program counter:** `36` -> `37`

**State before/after:**
- No register or unique-byte changes.

### Step 157

**P-Code:** `SUBPIECE unique_0x76600:8B, 0x4:4B -> EDX:4B`

**Program counter:** `37` -> `38`

**State before/after:**
- `EDX`: `0x0a (10)` -> `0x00 (0)`
- `RDX`: `0x0a (10)` -> `0x00 (0)`

### Step 158

**P-Code:** `INT_ZEXT EDX:4B -> RDX:8B`

**Program counter:** `38` -> `39`

**State before/after:**
- No register or unique-byte changes.

### Step 159

**P-Code:** `INT_LESS EAX:4B, EDX:4B -> CF:1B`

**Program counter:** `39` -> `40`

**State before/after:**
- No register or unique-byte changes.

### Step 160

**P-Code:** `INT_SBORROW EAX:4B, EDX:4B -> OF:1B`

**Program counter:** `40` -> `41`

**State before/after:**
- No register or unique-byte changes.

### Step 161

**P-Code:** `INT_SUB EAX:4B, EDX:4B -> EAX:4B`

**Program counter:** `41` -> `42`

**State before/after:**
- No register or unique-byte changes.

### Step 162

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `42` -> `43`

**State before/after:**
- No register or unique-byte changes.

### Step 163

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `43` -> `44`

**State before/after:**
- No register or unique-byte changes.

### Step 164

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `44` -> `45`

**State before/after:**
- No register or unique-byte changes.

### Step 165

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `45` -> `46`

**State before/after:**
- `unique_0x58300`: `0x01 (1)` -> `0x04 (4)`
- `unique_0x58300`: `0x01 (1)` -> `0x04 (4)`

### Step 166

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `46` -> `47`

**State before/after:**
- No register or unique-byte changes.

### Step 167

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `47` -> `48`

**State before/after:**
- No register or unique-byte changes.

### Step 168

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `48` -> `49`

**State before/after:**
- No register or unique-byte changes.

### Step 169

**P-Code:** `INT_AND EAX:4B, 0x1:4B -> unique_0xc2700:4B`

**Program counter:** `49` -> `50`

**State before/after:**
- No register or unique-byte changes.

### Step 170

**P-Code:** `INT_NOTEQUAL unique_0xc2700:4B, 0x0:4B -> CF:1B`

**Program counter:** `50` -> `51`

**State before/after:**
- No register or unique-byte changes.

### Step 171

**P-Code:** `COPY 0x0:1B -> OF:1B`

**Program counter:** `51` -> `52`

**State before/after:**
- No register or unique-byte changes.

### Step 172

**P-Code:** `INT_SRIGHT EAX:4B, 0x1:4B -> EAX:4B`

**Program counter:** `52` -> `53`

**State before/after:**
- `EAX`: `0x04 (4)` -> `0x02 (2)`
- `RAX`: `0x04 (4)` -> `0x02 (2)`

### Step 173

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `53` -> `54`

**State before/after:**
- No register or unique-byte changes.

### Step 174

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `54` -> `55`

**State before/after:**
- No register or unique-byte changes.

### Step 175

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `55` -> `56`

**State before/after:**
- No register or unique-byte changes.

### Step 176

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `56` -> `57`

**State before/after:**
- `unique_0x58300`: `0x04 (4)` -> `0x02 (2)`
- `unique_0x58300`: `0x04 (4)` -> `0x02 (2)`

### Step 177

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `57` -> `58`

**State before/after:**
- No register or unique-byte changes.

### Step 178

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `58` -> `59`

**State before/after:**
- No register or unique-byte changes.

### Step 179

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `59` -> `60`

**State before/after:**
- No register or unique-byte changes.

### Step 180

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x1`
**Program counter:** `60` -> `61`

**State before/after:**
- `unique_0x23d00`: `0x04 (4)` -> `0x01 (1)`

### Step 181

**P-Code:** `COPY unique_0x23d00:4B -> unique_0x79700:4B`

**Program counter:** `61` -> `62`

**State before/after:**
- `unique_0x79700`: `0x00 (0)` -> `0x01 (1)`

### Step 182

**P-Code:** `INT_LESS unique_0x79700:4B, EAX:4B -> CF:1B`

**Program counter:** `62` -> `63`

**State before/after:**
- `CF`: `0x00 (0)` -> `0x01 (1)`

### Step 183

**P-Code:** `INT_SBORROW unique_0x79700:4B, EAX:4B -> OF:1B`

**Program counter:** `63` -> `64`

**State before/after:**
- No register or unique-byte changes.

### Step 184

**P-Code:** `INT_SUB unique_0x79700:4B, EAX:4B -> unique_0x79900:4B`

**Program counter:** `64` -> `65`

**State before/after:**
- `unique_0x79900`: `0xfffffffe (4294967294)` -> `0xffffffff (4294967295)`

### Step 185

**P-Code:** `INT_SLESS unique_0x79900:4B, 0x0:4B -> SF:1B`

**Program counter:** `65` -> `66`

**State before/after:**
- `SF`: `0x00 (0)` -> `0x01 (1)`

### Step 186

**P-Code:** `INT_EQUAL unique_0x79900:4B, 0x0:4B -> ZF:1B`

**Program counter:** `66` -> `67`

**State before/after:**
- No register or unique-byte changes.

### Step 187

**P-Code:** `INT_AND unique_0x79900:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `67` -> `68`

**State before/after:**
- `unique_0x58300`: `0x02 (2)` -> `0xff (255)`
- `unique_0x58300`: `0x02 (2)` -> `0xff (255)`

### Step 188

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `68` -> `69`

**State before/after:**
- `unique_0x58400`: `0x01 (1)` -> `0x08 (8)`

### Step 189

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `69` -> `70`

**State before/after:**
- `unique_0x58500`: `0x01 (1)` -> `0x00 (0)`

### Step 190

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `70` -> `71`

**State before/after:**
- `PF`: `0x00 (0)` -> `0x01 (1)`

### Step 191

**P-Code:** `INT_EQUAL OF:1B, SF:1B -> unique_0x25c00:1B`

**Program counter:** `71` -> `72`

**State before/after:**
- No register or unique-byte changes.

### Step 192

**P-Code:** `CBRANCH ram[0x1400012ea]:8B, unique_0x25c00:1B`

**Control flow:** fall-through (target 152)
**Program counter:** `72` -> `73`

**State before/after:**
- No register or unique-byte changes.

### Step 193

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x1`
**Program counter:** `73` -> `74`

**State before/after:**
- No register or unique-byte changes.

### Step 194

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `74` -> `75`

**State before/after:**
- `EAX`: `0x02 (2)` -> `0x01 (1)`
- `RAX`: `0x02 (2)` -> `0x01 (1)`

### Step 195

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `75` -> `76`

**State before/after:**
- No register or unique-byte changes.

### Step 196

**P-Code:** `INT_ADD 0x28:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `76` -> `77`

**State before/after:**
- No register or unique-byte changes.

### Step 197

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200010/4B = 0x4`
**Program counter:** `77` -> `78`

**State before/after:**
- `unique_0x23d00`: `0x01 (1)` -> `0x04 (4)`

### Step 198

**P-Code:** `COPY unique_0x23d00:4B -> ECX:4B`

**Program counter:** `78` -> `79`

**State before/after:**
- `ECX`: `0x10000000 (268435456)` -> `0x04 (4)`
- `RCX`: `0x10000000 (268435456)` -> `0x04 (4)`

### Step 199

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `79` -> `80`

**State before/after:**
- No register or unique-byte changes.

### Step 200

**P-Code:** `INT_LESS ECX:4B, EAX:4B -> CF:1B`

**Program counter:** `80` -> `81`

**State before/after:**
- `CF`: `0x01 (1)` -> `0x00 (0)`

### Step 201

**P-Code:** `INT_SBORROW ECX:4B, EAX:4B -> OF:1B`

**Program counter:** `81` -> `82`

**State before/after:**
- No register or unique-byte changes.

### Step 202

**P-Code:** `INT_SUB ECX:4B, EAX:4B -> ECX:4B`

**Program counter:** `82` -> `83`

**State before/after:**
- `ECX`: `0x04 (4)` -> `0x03 (3)`
- `RCX`: `0x04 (4)` -> `0x03 (3)`

### Step 203

**P-Code:** `INT_ZEXT ECX:4B -> RCX:8B`

**Program counter:** `83` -> `84`

**State before/after:**
- No register or unique-byte changes.

### Step 204

**P-Code:** `INT_SLESS ECX:4B, 0x0:4B -> SF:1B`

**Program counter:** `84` -> `85`

**State before/after:**
- `SF`: `0x01 (1)` -> `0x00 (0)`

### Step 205

**P-Code:** `INT_EQUAL ECX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `85` -> `86`

**State before/after:**
- No register or unique-byte changes.

### Step 206

**P-Code:** `INT_AND ECX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `86` -> `87`

**State before/after:**
- `unique_0x58300`: `0xff (255)` -> `0x03 (3)`
- `unique_0x58300`: `0xff (255)` -> `0x03 (3)`

### Step 207

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `87` -> `88`

**State before/after:**
- `unique_0x58400`: `0x08 (8)` -> `0x02 (2)`

### Step 208

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `88` -> `89`

**State before/after:**
- No register or unique-byte changes.

### Step 209

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `89` -> `90`

**State before/after:**
- No register or unique-byte changes.

### Step 210

**P-Code:** `COPY ECX:4B -> EAX:4B`

**Program counter:** `90` -> `91`

**State before/after:**
- `EAX`: `0x01 (1)` -> `0x03 (3)`
- `RAX`: `0x01 (1)` -> `0x03 (3)`

### Step 211

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `91` -> `92`

**State before/after:**
- No register or unique-byte changes.

### Step 212

**P-Code:** `INT_SBORROW EAX:4B, 0x1:4B -> OF:1B`

**Program counter:** `92` -> `93`

**State before/after:**
- No register or unique-byte changes.

### Step 213

**P-Code:** `INT_SUB EAX:4B, 0x1:4B -> EAX:4B`

**Program counter:** `93` -> `94`

**State before/after:**
- `EAX`: `0x03 (3)` -> `0x02 (2)`
- `RAX`: `0x03 (3)` -> `0x02 (2)`

### Step 214

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `94` -> `95`

**State before/after:**
- No register or unique-byte changes.

### Step 215

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `95` -> `96`

**State before/after:**
- No register or unique-byte changes.

### Step 216

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `96` -> `97`

**State before/after:**
- No register or unique-byte changes.

### Step 217

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `97` -> `98`

**State before/after:**
- `unique_0x58300`: `0x03 (3)` -> `0x02 (2)`
- `unique_0x58300`: `0x03 (3)` -> `0x02 (2)`

### Step 218

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `98` -> `99`

**State before/after:**
- `unique_0x58400`: `0x02 (2)` -> `0x01 (1)`

### Step 219

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `99` -> `100`

**State before/after:**
- `unique_0x58500`: `0x00 (0)` -> `0x01 (1)`

### Step 220

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `100` -> `101`

**State before/after:**
- `PF`: `0x01 (1)` -> `0x00 (0)`

### Step 221

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `101` -> `102`

**State before/after:**
- `unique_0x9d00`: `0x200010 (2097168)` -> `0x1fffec (2097132)`

### Step 222

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `102` -> `103`

**State before/after:**
- `unique_0xd400`: `0x01 (1)` -> `0x02 (2)`

### Step 223

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffec/4B = 0x2`
**Program counter:** `103` -> `104`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffec`: `0x03 (3)` -> `0x02 (2)`

### Step 224

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x1`
**Program counter:** `104` -> `105`

**State before/after:**
- `unique_0x23d00`: `0x04 (4)` -> `0x01 (1)`

### Step 225

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RAX:8B`

**Program counter:** `105` -> `106`

**State before/after:**
- `EAX`: `0x02 (2)` -> `0x01 (1)`
- `RAX`: `0x02 (2)` -> `0x01 (1)`

### Step 226

**P-Code:** `INT_ADD 0x20:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `106` -> `107`

**State before/after:**
- `unique_0x9d00`: `0x1fffec (2097132)` -> `0x200008 (2097160)`

### Step 227

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200008/8B = 0x10000000`
**Program counter:** `107` -> `108`

**State before/after:**
- No register or unique-byte changes.

### Step 228

**P-Code:** `COPY unique_0x23e00:8B -> RCX:8B`

**Program counter:** `108` -> `109`

**State before/after:**
- `ECX`: `0x03 (3)` -> `0x10000000 (268435456)`
- `RCX`: `0x03 (3)` -> `0x10000000 (268435456)`

### Step 229

**P-Code:** `INT_MULT RAX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `109` -> `110`

**State before/after:**
- `unique_0x9300`: `0x0c (12)` -> `0x04 (4)`

### Step 230

**P-Code:** `INT_ADD RCX:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `110` -> `111`

**State before/after:**
- `unique_0x9500`: `0x1000000c (268435468)` -> `0x10000004 (268435460)`

### Step 231

**P-Code:** `LOAD 0x1b1:4B, unique_0x9500:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x10000004/4B = 0x14`
**Program counter:** `111` -> `112`

**State before/after:**
- `unique_0x23d00`: `0x01 (1)` -> `0x14 (20)`

### Step 232

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `112` -> `113`

**State before/after:**
- `EAX`: `0x01 (1)` -> `0x14 (20)`
- `RAX`: `0x01 (1)` -> `0x14 (20)`

### Step 233

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `113` -> `114`

**State before/after:**
- No register or unique-byte changes.

### Step 234

**P-Code:** `INT_ADD 0x8:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `114` -> `115`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x1ffff0 (2097136)`

### Step 235

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `115` -> `116`

**State before/after:**
- `unique_0xd400`: `0x02 (2)` -> `0x14 (20)`

### Step 236

**P-Code:** `STORE 0x1b1:4B, unique_0x9d00:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1ffff0/4B = 0x14`
**Program counter:** `116` -> `117`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1ffff0`: `0x0a (10)` -> `0x14 (20)`

### Step 237

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `117` -> `118`

**State before/after:**
- `unique_0x9d00`: `0x1ffff0 (2097136)` -> `0x1fffec (2097132)`

### Step 238

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffec/4B = 0x2`
**Program counter:** `118` -> `119`

**State before/after:**
- `unique_0x23d00`: `0x14 (20)` -> `0x02 (2)`

### Step 239

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RAX:8B`

**Program counter:** `119` -> `120`

**State before/after:**
- `EAX`: `0x14 (20)` -> `0x02 (2)`
- `RAX`: `0x14 (20)` -> `0x02 (2)`

### Step 240

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x1`
**Program counter:** `120` -> `121`

**State before/after:**
- `unique_0x23d00`: `0x02 (2)` -> `0x01 (1)`

### Step 241

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RCX:8B`

**Program counter:** `121` -> `122`

**State before/after:**
- `ECX`: `0x10000000 (268435456)` -> `0x01 (1)`
- `RCX`: `0x10000000 (268435456)` -> `0x01 (1)`

### Step 242

**P-Code:** `INT_ADD 0x20:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `122` -> `123`

**State before/after:**
- `unique_0x9d00`: `0x1fffec (2097132)` -> `0x200008 (2097160)`

### Step 243

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200008/8B = 0x10000000`
**Program counter:** `123` -> `124`

**State before/after:**
- No register or unique-byte changes.

### Step 244

**P-Code:** `COPY unique_0x23e00:8B -> RDX:8B`

**Program counter:** `124` -> `125`

**State before/after:**
- `EDX`: `0x00 (0)` -> `0x10000000 (268435456)`
- `RDX`: `0x00 (0)` -> `0x10000000 (268435456)`

### Step 245

**P-Code:** `INT_ADD 0x20:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `125` -> `126`

**State before/after:**
- No register or unique-byte changes.

### Step 246

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200008/8B = 0x10000000`
**Program counter:** `126` -> `127`

**State before/after:**
- No register or unique-byte changes.

### Step 247

**P-Code:** `COPY unique_0x23e00:8B -> R8:8B`

**Program counter:** `127` -> `128`

**State before/after:**
- No register or unique-byte changes.

### Step 248

**P-Code:** `INT_MULT RAX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `128` -> `129`

**State before/after:**
- `unique_0x9300`: `0x04 (4)` -> `0x08 (8)`

### Step 249

**P-Code:** `INT_ADD R8:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `129` -> `130`

**State before/after:**
- `unique_0x9500`: `0x10000004 (268435460)` -> `0x10000008 (268435464)`

### Step 250

**P-Code:** `LOAD 0x1b1:4B, unique_0x9500:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x10000008/4B = 0x1e`
**Program counter:** `130` -> `131`

**State before/after:**
- `unique_0x23d00`: `0x01 (1)` -> `0x1e (30)`

### Step 251

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `131` -> `132`

**State before/after:**
- `EAX`: `0x02 (2)` -> `0x1e (30)`
- `RAX`: `0x02 (2)` -> `0x1e (30)`

### Step 252

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `132` -> `133`

**State before/after:**
- No register or unique-byte changes.

### Step 253

**P-Code:** `INT_MULT RCX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `133` -> `134`

**State before/after:**
- `unique_0x9300`: `0x08 (8)` -> `0x04 (4)`

### Step 254

**P-Code:** `INT_ADD RDX:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `134` -> `135`

**State before/after:**
- `unique_0x9500`: `0x10000008 (268435464)` -> `0x10000004 (268435460)`

### Step 255

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `135` -> `136`

**State before/after:**
- `unique_0xd400`: `0x14 (20)` -> `0x1e (30)`

### Step 256

**P-Code:** `STORE 0x1b1:4B, unique_0x9500:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x10000004/4B = 0x1e`
**Program counter:** `136` -> `137`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x10000004`: `0x14 (20)` -> `0x1e (30)`
- `int[4] values` @ `0x10000000` (16 bytes): `40, 30, 30, 10`

### Step 257

**P-Code:** `INT_ADD 0x4:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `137` -> `138`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x1fffec (2097132)`

### Step 258

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffec/4B = 0x2`
**Program counter:** `138` -> `139`

**State before/after:**
- `unique_0x23d00`: `0x1e (30)` -> `0x02 (2)`

### Step 259

**P-Code:** `INT_SEXT unique_0x23d00:4B -> RAX:8B`

**Program counter:** `139` -> `140`

**State before/after:**
- `EAX`: `0x1e (30)` -> `0x02 (2)`
- `RAX`: `0x1e (30)` -> `0x02 (2)`

### Step 260

**P-Code:** `INT_ADD 0x20:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `140` -> `141`

**State before/after:**
- `unique_0x9d00`: `0x1fffec (2097132)` -> `0x200008 (2097160)`

### Step 261

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23e00:8B`

**Memory reads:** `ram:0x200008/8B = 0x10000000`
**Program counter:** `141` -> `142`

**State before/after:**
- No register or unique-byte changes.

### Step 262

**P-Code:** `COPY unique_0x23e00:8B -> RCX:8B`

**Program counter:** `142` -> `143`

**State before/after:**
- `ECX`: `0x01 (1)` -> `0x10000000 (268435456)`
- `RCX`: `0x01 (1)` -> `0x10000000 (268435456)`

### Step 263

**P-Code:** `INT_ADD 0x8:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `143` -> `144`

**State before/after:**
- `unique_0x9d00`: `0x200008 (2097160)` -> `0x1ffff0 (2097136)`

### Step 264

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1ffff0/4B = 0x14`
**Program counter:** `144` -> `145`

**State before/after:**
- `unique_0x23d00`: `0x02 (2)` -> `0x14 (20)`

### Step 265

**P-Code:** `COPY unique_0x23d00:4B -> EDX:4B`

**Program counter:** `145` -> `146`

**State before/after:**
- `EDX`: `0x10000000 (268435456)` -> `0x14 (20)`
- `RDX`: `0x10000000 (268435456)` -> `0x14 (20)`

### Step 266

**P-Code:** `INT_ZEXT EDX:4B -> RDX:8B`

**Program counter:** `146` -> `147`

**State before/after:**
- No register or unique-byte changes.

### Step 267

**P-Code:** `INT_MULT RAX:8B, 0x4:8B -> unique_0x9300:8B`

**Program counter:** `147` -> `148`

**State before/after:**
- `unique_0x9300`: `0x04 (4)` -> `0x08 (8)`

### Step 268

**P-Code:** `INT_ADD RCX:8B, unique_0x9300:8B -> unique_0x9500:8B`

**Program counter:** `148` -> `149`

**State before/after:**
- `unique_0x9500`: `0x10000004 (268435460)` -> `0x10000008 (268435464)`

### Step 269

**P-Code:** `COPY EDX:4B -> unique_0xd400:4B`

**Program counter:** `149` -> `150`

**State before/after:**
- `unique_0xd400`: `0x1e (30)` -> `0x14 (20)`

### Step 270

**P-Code:** `STORE 0x1b1:4B, unique_0x9500:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x10000008/4B = 0x14`
**Program counter:** `150` -> `151`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x10000008`: `0x1e (30)` -> `0x14 (20)`
- `int[4] values` @ `0x10000000` (16 bytes): `40, 30, 20, 10`

### Step 271

**P-Code:** `BRANCH ram[0x140001286]:8B`

**Control flow:** branch taken (target 18)
**Program counter:** `151` -> `18`

**State before/after:**
- No register or unique-byte changes.

### Step 272

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x1`
**Program counter:** `18` -> `19`

**State before/after:**
- `unique_0x23d00`: `0x14 (20)` -> `0x01 (1)`

### Step 273

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `19` -> `20`

**State before/after:**
- `EAX`: `0x02 (2)` -> `0x01 (1)`
- `RAX`: `0x02 (2)` -> `0x01 (1)`

### Step 274

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `20` -> `21`

**State before/after:**
- No register or unique-byte changes.

### Step 275

**P-Code:** `INT_SCARRY EAX:4B, 0x1:4B -> OF:1B`

**Program counter:** `21` -> `22`

**State before/after:**
- No register or unique-byte changes.

### Step 276

**P-Code:** `INT_ADD EAX:4B, 0x1:4B -> EAX:4B`

**Program counter:** `22` -> `23`

**State before/after:**
- `EAX`: `0x01 (1)` -> `0x02 (2)`
- `RAX`: `0x01 (1)` -> `0x02 (2)`

### Step 277

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `23` -> `24`

**State before/after:**
- No register or unique-byte changes.

### Step 278

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `24` -> `25`

**State before/after:**
- No register or unique-byte changes.

### Step 279

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `25` -> `26`

**State before/after:**
- No register or unique-byte changes.

### Step 280

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `26` -> `27`

**State before/after:**
- No register or unique-byte changes.

### Step 281

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `27` -> `28`

**State before/after:**
- No register or unique-byte changes.

### Step 282

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `28` -> `29`

**State before/after:**
- No register or unique-byte changes.

### Step 283

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `29` -> `30`

**State before/after:**
- No register or unique-byte changes.

### Step 284

**P-Code:** `COPY EAX:4B -> unique_0xd400:4B`

**Program counter:** `30` -> `31`

**State before/after:**
- `unique_0xd400`: `0x14 (20)` -> `0x02 (2)`

### Step 285

**P-Code:** `STORE 0x1b1:4B, RSP:8B, unique_0xd400:4B`

**Memory writes:** `ram:0x1fffe8/4B = 0x2`
**Program counter:** `31` -> `32`

**State before/after:**
- No register or unique-byte changes.

**Memory diff:**
- `ram:0x1fffe8`: `0x01 (1)` -> `0x02 (2)`

### Step 286

**P-Code:** `INT_ADD 0x28:8B, RSP:8B -> unique_0x9d00:8B`

**Program counter:** `32` -> `33`

**State before/after:**
- `unique_0x9d00`: `0x1ffff0 (2097136)` -> `0x200010 (2097168)`

### Step 287

**P-Code:** `LOAD 0x1b1:4B, unique_0x9d00:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x200010/4B = 0x4`
**Program counter:** `33` -> `34`

**State before/after:**
- `unique_0x23d00`: `0x01 (1)` -> `0x04 (4)`

### Step 288

**P-Code:** `COPY unique_0x23d00:4B -> EAX:4B`

**Program counter:** `34` -> `35`

**State before/after:**
- `EAX`: `0x02 (2)` -> `0x04 (4)`
- `RAX`: `0x02 (2)` -> `0x04 (4)`

### Step 289

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `35` -> `36`

**State before/after:**
- No register or unique-byte changes.

### Step 290

**P-Code:** `INT_SEXT EAX:4B -> unique_0x76600:8B`

**Program counter:** `36` -> `37`

**State before/after:**
- No register or unique-byte changes.

### Step 291

**P-Code:** `SUBPIECE unique_0x76600:8B, 0x4:4B -> EDX:4B`

**Program counter:** `37` -> `38`

**State before/after:**
- `EDX`: `0x14 (20)` -> `0x00 (0)`
- `RDX`: `0x14 (20)` -> `0x00 (0)`

### Step 292

**P-Code:** `INT_ZEXT EDX:4B -> RDX:8B`

**Program counter:** `38` -> `39`

**State before/after:**
- No register or unique-byte changes.

### Step 293

**P-Code:** `INT_LESS EAX:4B, EDX:4B -> CF:1B`

**Program counter:** `39` -> `40`

**State before/after:**
- No register or unique-byte changes.

### Step 294

**P-Code:** `INT_SBORROW EAX:4B, EDX:4B -> OF:1B`

**Program counter:** `40` -> `41`

**State before/after:**
- No register or unique-byte changes.

### Step 295

**P-Code:** `INT_SUB EAX:4B, EDX:4B -> EAX:4B`

**Program counter:** `41` -> `42`

**State before/after:**
- No register or unique-byte changes.

### Step 296

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `42` -> `43`

**State before/after:**
- No register or unique-byte changes.

### Step 297

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `43` -> `44`

**State before/after:**
- No register or unique-byte changes.

### Step 298

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `44` -> `45`

**State before/after:**
- No register or unique-byte changes.

### Step 299

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `45` -> `46`

**State before/after:**
- `unique_0x58300`: `0x02 (2)` -> `0x04 (4)`
- `unique_0x58300`: `0x02 (2)` -> `0x04 (4)`

### Step 300

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `46` -> `47`

**State before/after:**
- No register or unique-byte changes.

### Step 301

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `47` -> `48`

**State before/after:**
- No register or unique-byte changes.

### Step 302

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `48` -> `49`

**State before/after:**
- No register or unique-byte changes.

### Step 303

**P-Code:** `INT_AND EAX:4B, 0x1:4B -> unique_0xc2700:4B`

**Program counter:** `49` -> `50`

**State before/after:**
- No register or unique-byte changes.

### Step 304

**P-Code:** `INT_NOTEQUAL unique_0xc2700:4B, 0x0:4B -> CF:1B`

**Program counter:** `50` -> `51`

**State before/after:**
- No register or unique-byte changes.

### Step 305

**P-Code:** `COPY 0x0:1B -> OF:1B`

**Program counter:** `51` -> `52`

**State before/after:**
- No register or unique-byte changes.

### Step 306

**P-Code:** `INT_SRIGHT EAX:4B, 0x1:4B -> EAX:4B`

**Program counter:** `52` -> `53`

**State before/after:**
- `EAX`: `0x04 (4)` -> `0x02 (2)`
- `RAX`: `0x04 (4)` -> `0x02 (2)`

### Step 307

**P-Code:** `INT_ZEXT EAX:4B -> RAX:8B`

**Program counter:** `53` -> `54`

**State before/after:**
- No register or unique-byte changes.

### Step 308

**P-Code:** `INT_SLESS EAX:4B, 0x0:4B -> SF:1B`

**Program counter:** `54` -> `55`

**State before/after:**
- No register or unique-byte changes.

### Step 309

**P-Code:** `INT_EQUAL EAX:4B, 0x0:4B -> ZF:1B`

**Program counter:** `55` -> `56`

**State before/after:**
- No register or unique-byte changes.

### Step 310

**P-Code:** `INT_AND EAX:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `56` -> `57`

**State before/after:**
- `unique_0x58300`: `0x04 (4)` -> `0x02 (2)`
- `unique_0x58300`: `0x04 (4)` -> `0x02 (2)`

### Step 311

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `57` -> `58`

**State before/after:**
- No register or unique-byte changes.

### Step 312

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `58` -> `59`

**State before/after:**
- No register or unique-byte changes.

### Step 313

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `59` -> `60`

**State before/after:**
- No register or unique-byte changes.

### Step 314

**P-Code:** `LOAD 0x1b1:4B, RSP:8B -> unique_0x23d00:4B`

**Memory reads:** `ram:0x1fffe8/4B = 0x2`
**Program counter:** `60` -> `61`

**State before/after:**
- `unique_0x23d00`: `0x04 (4)` -> `0x02 (2)`

### Step 315

**P-Code:** `COPY unique_0x23d00:4B -> unique_0x79700:4B`

**Program counter:** `61` -> `62`

**State before/after:**
- `unique_0x79700`: `0x01 (1)` -> `0x02 (2)`

### Step 316

**P-Code:** `INT_LESS unique_0x79700:4B, EAX:4B -> CF:1B`

**Program counter:** `62` -> `63`

**State before/after:**
- No register or unique-byte changes.

### Step 317

**P-Code:** `INT_SBORROW unique_0x79700:4B, EAX:4B -> OF:1B`

**Program counter:** `63` -> `64`

**State before/after:**
- No register or unique-byte changes.

### Step 318

**P-Code:** `INT_SUB unique_0x79700:4B, EAX:4B -> unique_0x79900:4B`

**Program counter:** `64` -> `65`

**State before/after:**
- `unique_0x79900`: `0xffffffff (4294967295)` -> `0x00 (0)`

### Step 319

**P-Code:** `INT_SLESS unique_0x79900:4B, 0x0:4B -> SF:1B`

**Program counter:** `65` -> `66`

**State before/after:**
- No register or unique-byte changes.

### Step 320

**P-Code:** `INT_EQUAL unique_0x79900:4B, 0x0:4B -> ZF:1B`

**Program counter:** `66` -> `67`

**State before/after:**
- `ZF`: `0x00 (0)` -> `0x01 (1)`

### Step 321

**P-Code:** `INT_AND unique_0x79900:4B, 0xff:4B -> unique_0x58300:4B`

**Program counter:** `67` -> `68`

**State before/after:**
- `unique_0x58300`: `0x02 (2)` -> `0x00 (0)`
- `unique_0x58300`: `0x02 (2)` -> `0x00 (0)`

### Step 322

**P-Code:** `POPCOUNT unique_0x58300:4B -> unique_0x58400:1B`

**Program counter:** `68` -> `69`

**State before/after:**
- `unique_0x58400`: `0x01 (1)` -> `0x00 (0)`

### Step 323

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `69` -> `70`

**State before/after:**
- `unique_0x58500`: `0x01 (1)` -> `0x00 (0)`

### Step 324

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `70` -> `71`

**State before/after:**
- `PF`: `0x00 (0)` -> `0x01 (1)`

### Step 325

**P-Code:** `INT_EQUAL OF:1B, SF:1B -> unique_0x25c00:1B`

**Program counter:** `71` -> `72`

**State before/after:**
- `unique_0x25c00`: `0x00 (0)` -> `0x01 (1)`

### Step 326

**P-Code:** `CBRANCH ram[0x1400012ea]:8B, unique_0x25c00:1B`

**Control flow:** branch taken (target 152)
**Program counter:** `72` -> `152`

**State before/after:**
- No register or unique-byte changes.

### Step 327

**P-Code:** `INT_CARRY RSP:8B, 0x18:8B -> CF:1B`

**Program counter:** `152` -> `153`

**State before/after:**
- No register or unique-byte changes.

### Step 328

**P-Code:** `INT_SCARRY RSP:8B, 0x18:8B -> OF:1B`

**Program counter:** `153` -> `154`

**State before/after:**
- No register or unique-byte changes.

### Step 329

**P-Code:** `INT_ADD RSP:8B, 0x18:8B -> RSP:8B`

**Program counter:** `154` -> `155`

**State before/after:**
- `RSP`: `0x1fffe8 (2097128)` -> `0x200000 (2097152)`

### Step 330

**P-Code:** `INT_SLESS RSP:8B, 0x0:8B -> SF:1B`

**Program counter:** `155` -> `156`

**State before/after:**
- No register or unique-byte changes.

### Step 331

**P-Code:** `INT_EQUAL RSP:8B, 0x0:8B -> ZF:1B`

**Program counter:** `156` -> `157`

**State before/after:**
- `ZF`: `0x01 (1)` -> `0x00 (0)`

### Step 332

**P-Code:** `INT_AND RSP:8B, 0xff:8B -> unique_0x58300:8B`

**Program counter:** `157` -> `158`

**State before/after:**
- No register or unique-byte changes.

### Step 333

**P-Code:** `POPCOUNT unique_0x58300:8B -> unique_0x58400:1B`

**Program counter:** `158` -> `159`

**State before/after:**
- No register or unique-byte changes.

### Step 334

**P-Code:** `INT_AND unique_0x58400:1B, 0x1:1B -> unique_0x58500:1B`

**Program counter:** `159` -> `160`

**State before/after:**
- No register or unique-byte changes.

### Step 335

**P-Code:** `INT_EQUAL unique_0x58500:1B, 0x0:1B -> PF:1B`

**Program counter:** `160` -> `161`

**State before/after:**
- No register or unique-byte changes.

### Step 336

**P-Code:** `LOAD 0x1b1:8B, RSP:8B -> RIP:8B`

**Memory reads:** `ram:0x200000/8B = 0xdeadbeef`
**Program counter:** `161` -> `162`

**State before/after:**
- `RIP`: `<unmapped>` -> `0xdeadbeef (3735928559)`

### Step 337

**P-Code:** `INT_ADD RSP:8B, 0x8:8B -> RSP:8B`

**Program counter:** `162` -> `163`

**State before/after:**
- `RSP`: `0x200000 (2097152)` -> `0x200008 (2097160)`

### Step 338

**P-Code:** `RETURN RIP:8B`

**Control flow:** return
**Program counter:** `163` -> `163`

**State before/after:**
- No register or unique-byte changes.

## Final State

- Program counter: `163`
- Halted: `True`

- `int[4] values` @ `0x10000000` (16 bytes): `40, 30, 20, 10`

## Validation

**Expected result:**
`[40, 30, 20, 10]`

**Normal C++ result:**
`[40, 30, 20, 10]`

**Emulated result:**
`[40, 30, 20, 10]`

**Status:** `PASS`
