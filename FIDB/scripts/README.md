# Pipeline Scripts

The scripts in this directory implement the four explicit stages documented in [`../README.md`](../README.md): [`build_zlib.py`](build_zlib.py), [`analyze_library.py`](analyze_library.py), [`create_fidb.py`](create_fidb.py), and [`verify_fidb.py`](verify_fidb.py). [`common.py`](common.py) contains only shared paths, configuration loading, tool discovery, and diagnostics; it does not implement FID hashing or database serialization.

PyGhidra scripts are launched through [`../../TEST/run_ghidra_python.bat`](../../TEST/run_ghidra_python.bat). The Ghidra stages use the installed Java classes and the original Function ID Java prescript, while the native build stage invokes the upstream zlib makefile under x64 MSVC.
