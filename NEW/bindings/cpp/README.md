# C++ Binding Surface

This directory is the native C++ facade prepared for future language bindings.

- [`runtime.cppm`](runtime.cppm) owns the process runtime handle.
- [`project.cppm`](project.cppm) owns a facade-only project handle.
- [`commands.cppm`](commands.cppm) exposes typed core commands/results.
- [`queries.cppm`](queries.cppm) exposes stable read-only project views.
- [`results.cppm`](results.cppm) exposes copied result values.
- [`debugger.cppm`](debugger.cppm) adapts the backend-independent debugger contract without importing a concrete backend or native DbgEng types.

Python, JavaScript, and Go bindings are intentionally not implemented in this task.
