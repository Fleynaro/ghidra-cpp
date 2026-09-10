# Agent Rules

## Ghidra Documentation

- The [`/docs`](docs/) directory is exclusively for the Ghidra Java project: its architecture, modules, dependencies, and runtime behavior. Do not use it to document the separate C++ project.
- Before changing Ghidra code, read the relevant `README.md` files and `.md` documents. For architecture or module-navigation tasks, start with [`docs/ARCHITECTURE_OVERVIEW_RU.md`](docs/ARCHITECTURE_OVERVIEW_RU.md) and verify documentation against the source code.
- Update the relevant Ghidra documentation when a change affects Ghidra architecture, build process, configuration, or user-visible behavior. Follow the source code when documentation conflicts with it.
- Ghidra documentation must contain relative references to the source files, directories, tests, and configuration it describes. Specialized documents must link back to the architecture overview, and the overview must link to specialized documents. Verify all added links before completing the task.
- Record verified answers about Ghidra architecture, module dependencies, or source implementation in [`docs/FAQ_RU.md`](docs/FAQ_RU.md), with references to current code and related documentation.

## C++ Project Documentation

- The C++ project developed alongside Ghidra is a separate project, even though it resides in the same repository. Document it in its own directories, not in `/docs`.
- Whenever a new C++ module is created, meaning a new directory or subdirectory, create an English `README.md` in that directory.
- Each C++ module `README.md` must comprehensively describe the module and include relative references to its source files, parent and child directories, configuration, dependencies, build scripts, and tests. Documentation must provide in-place navigation through the module hierarchy.
- Keep every C++ module `README.md` synchronized with its code. Update related documentation whenever the module's structure, behavior, interfaces, configuration, workflow, or dependencies change.

## Environment Variables

The project uses environment variables. Keep this table up to date.

| File | Variable | Purpose |
|---|---|---|
| `.env` | `GHIDRA_INSTALL_DIR` | Path to the installed Ghidra software. |

- Before running builds, tests, or tools, verify that the required environment variables exist and contain valid values.
- Never hard-code environment-variable values in source code.
- Never commit secrets, machine-specific paths, or other local environment values to Git.
- When adding a new environment variable, add it to the table above and document its purpose.

## C++

- Use `vcpkg` as the package manager for C++ development and builds.
- Do not add third-party C++ dependencies manually when they can be installed and integrated through `vcpkg`.
- Before adding a dependency, check whether the required package and its `vcpkg` integration already exist in the project.
- Use CMake, the project's `CMakeLists.txt`, and Ninja for builds.
- Run builds through the appropriate `build.bat` script rather than invoking an ad hoc build workflow.
- Use the MSVC compiler unless the task explicitly requires another compiler.
- Compile with C++23 enabled and use modern C++23 features where they improve correctness, clarity, or maintainability.
- Prefer the C++ module system for imports over legacy `#include` directives whenever practical.
- Use the `.cppm` extension for C++ module interface files; do not use `.ixx`.
- Format C++ code with `clang-format` and validate it with `clang-tidy`.

## Code Quality

- Write modern, clear, well-structured code with sound architecture.
- All code, strings, user-facing messages, GUI text, comments, documentation embedded in source code, identifiers, and other project content must be in English.
- Use thorough error handling. Handle every realistic failure point and present users with clear English diagnostics, including recommended remediation steps when applicable.
- Use strong typing and express intent through appropriate types, interfaces, and names.
- Keep code readable and understandable. Avoid unnecessary complexity, unclear abbreviations, and duplication.
- Add comments where they improve understanding of the code.
- Source-code comments and descriptions must be written in English and explain purpose, constraints, invariants, or non-obvious decisions rather than merely restating the implementation.
- Preserve existing architecture and conventions unless the task explicitly requires changing them.
- Keep changes focused and avoid unrelated refactoring.

## Validation

- Before considering a task complete, build the affected targets and run the relevant tests or validation tools.
- Run formatting and static analysis for modified C++ code.
- Review the final diff for accidental files, hard-coded local values, secrets, non-English content, and missing documentation updates.
- Report validation failures clearly instead of hiding or ignoring them.
