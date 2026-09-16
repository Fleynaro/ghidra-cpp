# Project Tests

[`project_tests.cppm`](project_tests.cppm) is the focused facade integration test. It opens `test_analyzers_integration.exe`, loads/decodes it through services, runs the scheduler, routes a rename command through the commit lane, and requests decompilation.
