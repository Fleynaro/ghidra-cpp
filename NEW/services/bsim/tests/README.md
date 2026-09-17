# BSim Tests

`bsim_tests.cppm` covers fixed-width signature hashing, duplicate-preserving
TF construction, cosine/significance behavior, deterministic graph features,
and the fixture target. `bsim_similarity_integration_tests.cppm` runs the real
PE Loader -> Sleigh -> native Decompiler -> BSim pipeline and checks five
semantic-twin pairs against an unrelated function.

The fixture source and reproducible MSVC wrapper are under [`data/`](data/).

The test target is registered by [`CMakeLists.txt`](CMakeLists.txt) and links
the public `ReCode::Bsim` service contract plus Google Test. Run it with
`NEW\build.bat bsim` or `ctest --test-dir NEW\build -R bsim`.
