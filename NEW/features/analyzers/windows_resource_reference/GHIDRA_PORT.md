# Ghidra Port Evidence

- Analyzer: [`Ghidra/Features/MicrosoftCodeAnalyzer/src/main/java/ghidra/app/plugin/prototype/MicrosoftCodeAnalyzerPlugin/WindowsResourceReferenceAnalyzer.java`](../../../../Ghidra/Features/MicrosoftCodeAnalyzer/src/main/java/ghidra/app/plugin/prototype/MicrosoftCodeAnalyzerPlugin/WindowsResourceReferenceAnalyzer.java).
- Helper script: [`Ghidra/Features/Decompiler/ghidra_scripts/WindowsResourceReference.java`](../../../../Ghidra/Features/Decompiler/ghidra_scripts/WindowsResourceReference.java).
- Native source: [`src/windows_resource_reference.cppm`](src/windows_resource_reference.cppm).
- Native tests: [`tests/windows_resource_reference_tests.cppm`](tests/windows_resource_reference_tests.cppm).

The port retains the analyzer name, one-time/data-type priority `900`, PE-only provider boundary,
the original Windows lookup API families including string, accelerator, bitmap, icon, dialog, menu,
cursor, image, MUI, wave, and type-library calls, resource type IDs, string-table `(id - 1) / 16 + 1` selection,
UTF-16 string-table entry offsets, DATA reference creation, duplicate handling, and optional
analysis bookmarks.

The Java helper follows arbitrary interprocedural definitions through a decompiler HighFunction and
tracks a persistent `WindowsResourceChecked` property map. The native public API provides neither a
Decompiler nor a property-map store. The port instead consumes existing direct references and
constant facts and intentionally does not pretend to recover values that are not already public.
