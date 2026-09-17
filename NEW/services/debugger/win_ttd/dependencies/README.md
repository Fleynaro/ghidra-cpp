# Windows TTD Dependencies

This directory is the self-contained native dependency boundary for [`../win_ttd.cppm`](../win_ttd.cppm) and [`../recorder/win_ttd.cppm`](../recorder/win_ttd.cppm). It intentionally has no runtime or build dependency on the former external reference workspace.

- [`packages.config`](packages.config) pins `Microsoft.TimeTravelDebugging.Apis` 0.9.5.
- [`Microsoft.TimeTravelDebugging.Apis.0.9.5`](Microsoft.TimeTravelDebugging.Apis.0.9.5) contains the copied TTD headers, CMake import-target definition, and Windows import libraries. The package's Microsoft license/signature files are retained with the copied package.
- [`runtime/x64`](runtime/x64) contains the two runtime DLLs required by the x64 Replay API: `TTDReplay.dll` and `TTDReplayCPU.dll`.
- [`Get-TtdReplayRuntime.ps1`](Get-TtdReplayRuntime.ps1) downloads those DLLs from Microsoft's current TTD MSIX distribution when refreshing the local copy.
- [`../setup_dependencies.bat`](../setup_dependencies.bat) implements provisioning, exposed through the single recommended `NEW\build.bat ttd_setup` CMake/build target.

CMake defaults to these service-local paths. `TTD_APIS_PACKAGE_DIR` and `TTD_RUNTIME_DIR` are supported only as explicit overrides for another installation. The checked-in binaries are x64 because `NEW/build.bat` configures the `x64-windows` toolchain; the package retains its other import-library architectures for future build modes.
