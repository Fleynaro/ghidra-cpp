# Aggregate Analyzer Fixture

The files in this directory build one normal Windows x64 PE that resembles a
small non-graphical game engine. It intentionally combines the positive and
negative inputs from the focused analyzer fixtures instead of concatenating
their sources.

## Contents

- [`test_analyzers_integration.cpp`](test_analyzers_integration.cpp) defines the
  Entity, Vehicle, Ped, Player, Asset, TextureAsset, and ScriptAsset hierarchies;
  recursive/mutually recursive functions; switches; callbacks; imports; media;
  strings; tables; globals; stack locals; dead code; and filler boundaries.
- [`test_analyzers_integration.rc`](test_analyzers_integration.rc) creates real
  Windows string-table, dialog, and menu resources.
- [`resource.h`](resource.h) defines resource identifiers used by the source.
- [`build.bat`](build.bat) invokes MSVC, the Windows resource compiler, and the
  linker with `/NODEFAULTLIB`, `/OPT:NOREF`, `/OPT:NOICF`, and `/DEBUG:FULL`.
- `test_analyzers_integration.exe` and its PDB are generated inputs, not runtime
  report data.

The fixture has no graphics, network, timing, randomness, templates, or
third-party dependencies. `GetTickCount` is retained only as an imported API
reference; the test never executes the fixture.
