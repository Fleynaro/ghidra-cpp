# Test 1
bytes: `40 53 48 83 ec 20 45 33 d2 4c 8b c9 48 85 c9 74 0e 48 85 d2 74 09 4d 85 c0 75 1d 66 44 89 11 e8 c8 5c 00 00 bb 16 00 00 00 89 18 e8 2c ac 00 00 8b c3 48 83 c4 20 5b c3 66 44 39 11 74 09 48 83 c1 02 48 ff ca 75 f1 48 85 d2 75 06 66 45 89 11 eb cd 49 2b c8 41 0f b7 00 66 42 89 04 01 4d 8d 40 02 66 85 c0 74 05 48 ff ca 75 e9 48 85 d2 75 10 66 45 89 11 e8 72 5c 00 00 bb 22 00 00 00 eb a8 33 c0 eb ad`

compiler: `Visual Studio 2012 Release`

function: `errno_t __cdecl wcscat_s(wchar_t *_Dst,rsize_t _SizeInWords,wchar_t *_Src)`



    
# Test 2
bytes: `48 89 5c 24 08 48 89 54 24 10 57 48 83 ec 20 48 8b da 8b f9 33 c0 48 85 d2 0f 95 c0 85 c0 75 15 e8 17 5b fe ff c7 00 16 00 00 00 e8 7c aa fe ff 83 c8 ff eb 1f 48 8b ca e8 4f 98 fd ff 90 48 8b d3 8b cf e8 b0 fe ff ff 8b f8 48 8b cb e8 d6 98 fd ff 8b c7 48 8b 5c 24 30 48 83 c4 20 5f c3`

compiler: `Visual Studio 2010 Release, Visual Studio 2015 Release`

function: `int __cdecl ungetc(int _Ch,FILE *_File)`




# Test 3
bytes: `48 89 5c 24 08 48 89 74 24 10 57 48 83 ec 20 48 8b d9 48 83 f9 e0 77 7c bf 01 00 00 00 48 85 c9 48 0f 45 f9 48 8b 0d f5 93 5d 01 48 85 c9 75 20 e8 e3 54 00 00 b9 1e 00 00 00 e8 4d 55 00 00 b9 ff 00 00 00 e8 b3 1d 00 00 48 8b 0d d0 93 5d 01 4c 8b c7 33 d2 ff 15 e5 6d 07 00 48 8b f0 48 85 c0 75 2c 39 05 ef 9e 5d 01 74 0e 48 8b cb e8 a5 f5 00 00 85 c0 74 0d eb ab e8 9e 1a 00 00 c7 00 0c 00 00 00 e8 93 1a 00 00 c7 00 0c 00 00 00 48 8b c6 eb 12 e8 7f f5 00 00 e8 7e 1a 00 00 c7 00 0c 00 00 00 33 c0 48 8b 5c 24 30 48 8b 74 24 38 48 83 c4 20 5f c3`

compiler: `Visual Studio 2010 Release, Visual Studio 2012 Release`

function: `void * __cdecl malloc(size_t _Size)`




# Test 4
bytes: `48 2b d1 49 83 f8 08 72 22 f6 c1 07 74 14 66 90 8a 01 3a 04 0a 75 2c 48 ff c1 49 ff c8 f6 c1 07 75 ee 4d 8b c8 49 c1 e9 03 75 1f 4d 85 c0 74 0f 8a 01 3a 04 0a 75 0c 48 ff c1 49 ff c8 75 f1 48 33 c0 c3 1b c0 83 d8 ff c3 90 49 c1 e9 02 74 37 48 8b 01 48 3b 04 0a 75 5b 48 8b 41 08 48 3b 44 0a 08 75 4c 48 8b 41 10 48 3b 44 0a 10 75 3d 48 8b 41 18 48 3b 44 0a 18 75 2e 48 83 c1 20 49 ff c9 75 cd 49 83 e0 1f 4d 8b c8 49 c1 e9 03 74 9b 48 8b 01 48 3b 04 0a 75 1b 48 83 c1 08 49 ff c9 75 ee 49 83 e0 07 eb 83 48 83 c1 08 48 83 c1 08 48 83 c1 08 48 8b 0c 11 48 0f c8 48 0f c9 48 3b c1 1b c0 83 d8 ff c3`

compiler: `Visual Studio`

function: `int __cdecl memcmp(void *_Buf1,void *_Buf2,size_t _Size)`




# Test 5 (Conflict - multiple match)
bytes: `48 8b c4 48 89 48 08 48 89 50 10 4c 89 40 18 4c 89 48 20 53 57 48 83 ec 28 33 c0 48 85 c9 0f 95 c0 85 c0 75 15 e8 12 4b 00 00 c7 00 16 00 00 00 e8 77 9a 00 00 83 c8 ff eb 6a 48 8d 7c 24 48 e8 40 88 ff ff 48 8d 50 30 b9 01 00 00 00 e8 a2 88 ff ff 90 e8 2c 88 ff ff 48 8d 48 30 e8 17 23 01 00 8b d8 e8 1c 88 ff ff 48 8d 48 30 4c 8b cf 45 33 c0 48 8b 54 24 40 e8 98 d5 00 00 8b f8 e8 01 88 ff ff 48 8d 50 30 8b cb e8 b2 22 01 00 90 e8 f0 87 ff ff 48 8d 50 30 b9 01 00 00 00 e8 d6 88 ff ff 8b c7 48 83 c4 28 5f 5b c3`

compiler: `Visual Studio 2012 Release`

function: `printf / wprintf`