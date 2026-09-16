# End-to-End Integration

[`end_to_end_tests.cppm`](end_to_end_tests.cppm) uses `features/analyzers/tests/data/test_analyzers_integration.exe` as the primary executable fixture. It opens a project through the native facade, loads PE state, decodes the entry body, runs scheduler-backed analysis, queries a selected function, and requests decompilation.
