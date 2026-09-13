# Agent Rules

## C++23 Ghidra Port

- This repository is a fork of the original Ghidra source tree. The project goal is to study the original Ghidra Java and C sources and build an independent, end-to-end autonomous Ghidra implementation in modern C++23 under `{workdir}/NEW` (the repository's `NEW/` directory).
- Port the implementation module by module, preserving the original functionality 1:1. Do not remove, simplify, stub, or otherwise cut original behavior merely to make a module easier to implement.
- Every ported C++23 module must be as autonomous as practical, with explicit boundaries, minimal coupling, and all runtime behavior required for its own operation. A module must not depend on the original Java or C implementation at runtime unless this is explicitly documented as a temporary migration step.
- The `NEW/framework` directory contains the core model. It is intentionally limited for now and currently focuses on p-code; it must not assume the original database or other Ghidra subsystems. The long-term core model is an RDF-like system of facts and hypotheses.
- The `NEW/features` directory contains feature modules such as the Sleigh runtime p-code decoder, PE loader, decompiler, and future equivalents of other Ghidra capabilities.
- Every feature module must contain a `tests` directory with thorough Google Test coverage. Tests must use one consistent format, include meaningful English comments, and verify behavior and edge cases against the original Ghidra contract.
- Source files in every C++23 module must include comments with relative references to the corresponding original Ghidra source files, classes, methods, algorithms, or documentation. These references are mandatory and must make the origin of the ported behavior traceable.
- Keep porting evidence and behavior mappings explicit: when the original implementation has non-obvious invariants, compatibility rules, serialization details, or error behavior, document them in the C++23 source comments and tests.
- When creating or changing a module, keep its documentation, build configuration, dependencies, source-to-original references, and Google Test coverage synchronized with the implementation.

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

## PyGhidra Script Execution

- Every PyGhidra Python script, including test and diagnostic scripts, must be launched through [`TEST/run_ghidra_python.bat`](TEST/run_ghidra_python.bat).
- Use `TEST\run_ghidra_python.bat <script.py> [script arguments...]` from the repository root, or pass the script path relative to the `TEST` directory when invoking the wrapper from another directory.
- Do not launch PyGhidra scripts directly with `python`, `python.exe`, `pyghidraRun`, or `pyghidraRun.bat`; the wrapper is required because it selects and validates the PyGhidra virtual environment and forwards the script arguments consistently.

## C++

- Use `vcpkg` as the package manager for C++ development and builds.
- Do not add third-party C++ dependencies manually when they can be installed and integrated through `vcpkg`.
- Before adding a dependency, check whether the required package and its `vcpkg` integration already exist in the project.
- Use CMake, the project's `CMakeLists.txt`, and Ninja for builds.
- Run builds through the appropriate `build.bat` script rather than invoking an ad hoc build workflow.
- Keep the normal C++ development loop fast: do not delete `NEW/build` for ordinary changes, and use the narrowest `NEW/build.bat` mode that covers the changed feature.
- `NEW/build.bat` defaults to the complete build and test workflow. Use the module-local `NEW\features\<feature>\build.bat` wrapper, or `NEW\build.bat hello`, `NEW\build.bat sleigh`, `NEW\build.bat pe`, `NEW\build.bat function_id`, or `NEW\build.bat decompiler`, to incrementally build and test one feature. Tests are enabled by default; add `--no-test` only when compilation without tests is required.
- Use `NEW\build.bat all` only for an integration check; it builds every target and runs every registered CTest test. Use `--clean` only after toolchain or CMake changes, or when a clean rebuild is specifically required.
- When creating a feature, add its library and executable targets to the appropriate `NEW/features/<feature>/CMakeLists.txt`, add a `tests` target registered with CTest, and add or update the feature `README.md` files and source-to-original references in the same change.
- After creating a feature, configure or refresh the preserved build with `NEW\build.bat <feature> --clean` only if the target graph changed; then use `NEW\build.bat <feature>` for subsequent edits. Verify the executable or library target with `NEW\build.bat <feature> --no-test` when a test run is not needed.
- Before considering a feature complete, run its focused build and tests, inspect the test list with `ctest --test-dir NEW\build -N` when diagnosing registration, and run `NEW\build.bat all` for the final full-project build and test pass.
- Use the MSVC compiler unless the task explicitly requires another compiler.
- Compile with C++23 enabled and use modern C++23 features where they improve correctness, clarity, or maintainability.
- Always use `import std;` for the C++ standard library; do not use `#include` directives for standard library headers.
- Prefer the C++ module system for imports over legacy `#include` directives whenever practical.
- Use the `.cppm` extension for C++ module interface files; do not use `.ixx`.
- Format C++ code with `clang-format` and validate it with `clang-tidy` through the repository wrappers [`NEW/format.bat`](NEW/format.bat) and [`NEW/tidy.bat`](NEW/tidy.bat), not by invoking the tools with an ad hoc file list. Both scripts default to `all` and accept one module selector: `all`, `hello`, `sleigh`, `pe`, `function_id`, or `decompiler`. Each module also provides local `format.bat` and `tidy.bat` wrappers that select the corresponding root module mode.
- Run `NEW\format.bat [module]` to format the selected module in place. Agents must run only `NEW\tidy.bat [module] --check` for read-only clang-tidy validation and must not run the fix mode; applying clang-tidy fixes is reserved for the user. Before finalizing C++ changes, format the affected module and run its tidy check, then inspect the diff for unintended formatting or fixes.
- When a function must be decompiled from supplied hex bytes, especially while developing or testing a new decompiler feature, use the standalone decompiler CLI rather than coupling the check to test fixtures or GoogleTest. Follow the argument grammar and reproducible examples in [`NEW/features/decompiler/cli/README.md`](NEW/features/decompiler/cli/README.md), and include the relevant `.sla`, address/range, hex bytes, and provider metadata in the command.

## Code Quality

- Write modern, clear, well-structured code with sound architecture.
- All code, strings, user-facing messages, GUI text, comments, documentation embedded in source code, identifiers, and other project content must be in English.
- Use thorough error handling. Handle every realistic failure point and present users with clear English diagnostics, including recommended remediation steps when applicable.
- Use strong typing and express intent through appropriate types, interfaces, and names.
- Keep code readable and understandable. Avoid unnecessary complexity, unclear abbreviations, and duplication.
- Preserve existing architecture and conventions unless the task explicitly requires changing them.
- Keep changes focused and avoid unrelated refactoring.

## Code Comments and Test Documentation

- Comments are mandatory, not optional. Do not consider an implementation complete until the required comments have been added and reviewed.
- Every function, method, constructor, destructor, class, struct, and test fixture must have an English documentation comment immediately before its declaration or definition. This includes private and otherwise apparently self-explanatory code.
- Each function or method comment must state its purpose and, where applicable, document parameters, return values, thrown errors, side effects, ownership or lifetime rules, preconditions, postconditions, and important invariants.
- Each class or struct comment must explain its responsibility, lifecycle or ownership model, important invariants, and intended usage.
- Every test case and test helper must have an English comment explaining the behavior or contract being verified, the relevant setup, and why the assertions matter. Comments must describe the scenario and expected outcome, not merely repeat the test name or assertion.
- Add comments to non-obvious algorithms, synchronization, platform-specific workarounds, serialization formats, protocol details, and security-sensitive code. Explain the reason, constraint, or invariant rather than paraphrasing the code.
- Place documentation comments where the reader encounters them before the related declaration or definition. Keep comments synchronized with the implementation whenever behavior changes.
- Before finalizing a change, explicitly inspect every new or modified function, method, class, struct, test fixture, test case, and test helper for a corresponding meaningful comment. Missing required comments are a validation failure.
- Source-code comments and descriptions must be written in English and explain purpose, constraints, invariants, or non-obvious decisions rather than merely restating the implementation.

## Validation

- Before considering a task complete, build the affected targets and run the relevant tests or validation tools.
- Run formatting and static analysis for modified C++ code.
- Review the final diff for accidental files, hard-coded local values, secrets, non-English content, and missing documentation updates.
- Report validation failures clearly instead of hiding or ignoring them.
