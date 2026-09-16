# Strict PE Loader API Audit

## Scope

- [x] Reviewed [`src/pe_loader.cppm`](src/pe_loader.cppm), its public `LoadedPeImage` address/memory APIs, and analyzer callers under [`../analyzers/`](../analyzers/).
- [x] Compared section mapping and forwarded-export behavior with [`PeLoader.java`](../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion/PeLoader.java).
- [x] Reviewer: Kilo.
- [x] Review date: 2026-09-15.
- [x] Review type: strict read-only source audit; no implementation or configuration changes were made.
- [x] Validation included `git diff --check`, `GHIDRA_INSTALL_DIR` verification, and `ctest --test-dir NEW/build -N`.
- [x] Current core-contract boundary addendum reviewed; its detailed findings are recorded in [`../REVIEW_REPORT.md`](../REVIEW_REPORT.md) and [`../../core/contracts/REVIEW_REPORT.md`](../../core/contracts/REVIEW_REPORT.md).

## Findings

### Critical

No findings.

### High

#### PE-HIGH-001: Section memory regions misrepresent raw-less and aligned PE section tails

- [ ] Remediation complete.
- Severity: high.
- Title: The loader marks all section memory as initialized and uses unaligned loaded sizes.
- Source: [`src/pe_loader.cppm:1675-1689`](src/pe_loader.cppm#L1675-L1689), [`src/pe_loader.cppm:1823-1842`](src/pe_loader.cppm#L1823-L1842), symbols `Parser::parse_sections` and `Parser::map_image`.
- Affected component: `MemoryRegion::initialized`, section range coverage, ASCII/media/pointer scans, and Sleigh decoding at aligned virtual tails.
- Technical evidence: `section.loaded_size` is assigned `max(virtual_size, raw_size)` rather than the SectionAlignment-rounded extent, and `MemoryRegion{..., true, false, ...}` hardcodes `initialized=true` for every section. The original loader uses `createInitializedBlock` for raw-backed bytes and `createUninitializedBlock` when no initialized block exists at [`PeLoader.java:645-682`](../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion/PeLoader.java#L645-L682).
- Expected behavior: a valid PE section with no raw bytes must expose its aligned virtual extent as uninitialized memory; a raw-backed section must retain the loader's aligned block extent. Consumers must be able to distinguish file-backed bytes from zero-filled/uninitialized tails.
- Actual behavior: a raw-less `.bss`-style section of virtual size `0x10` and SectionAlignment `0x1000` exposes only `0x10` bytes and reports them initialized. Analyzer scans therefore treat virtual zero fill as loaded file data, while valid addresses in the aligned tail are rejected by `find_memory_region`.
- Impact: false strings, media, and address tables can be created in uninitialized memory; code/data at valid aligned tail addresses cannot be decoded; PDB/RVA consumers see a memory model different from Ghidra.
- Reproduction or failure scenario: load a PE section with `raw_size=0`, `virtual_size=0x10`, and `section_alignment=0x1000`. `memory_regions()` returns a `0x10`-byte initialized region instead of the uninitialized aligned block, and `read_memory`/`find_memory_region` disagree with the original memory model.
- Root cause: virtual extent and initialization provenance were collapsed into one `loaded_size` field and a hardcoded initialization flag.
- Recommended fix: retain separate aligned block extent, file-backed initialized extent, and uninitialized virtual extent in the loader API; construct region metadata from those facts and preserve gaps as distinct memory blocks where required.
- Regression risks: changing ranges affects analyzer seed discovery and overlap tests; validate raw-backed tails, raw-less sections, zero virtual size sections, and SectionAlignment edge cases.
- Relevant tests or validation: current PE tests cover ordinary mapped fixtures but no raw-less aligned section with assertions for `initialized` and full virtual extent.

### Medium

#### PE-MEDIUM-001: Forwarded exports have no external address-space side effect

- [ ] Remediation complete.
- Severity: medium.
- Title: `ExportedSymbol::forwarded` is retained as metadata but not materialized as an external symbol/block.
- Source: [`src/pe_loader.cppm:362-371`](src/pe_loader.cppm#L362-L371), [`src/pe_loader.cppm:719-722`](src/pe_loader.cppm#L719-L722), [`../analyzers/external_entry_references/src/external_entry_references.cppm:48-56`](../analyzers/external_entry_references/src/external_entry_references.cppm#L48-L56), symbols `ExportedSymbol` and `ExternalEntryReferencesAnalyzer`.
- Affected component: forwarded DLL exports, external namespace resolution, external-entry references, and imported API identity.
- Technical evidence: the loader stores a forwarder string and marks the export `forwarded`, but exposes only headers/sections as memory regions. The analyzer skips forwarded exports at line 52. Original `PeLoader` allocates an `EXTERNAL` block for forwarded exports at [`PeLoader.java:535-545`](../../../Ghidra/Features/Base/src/main/java/ghidra/app/util/opinion/PeLoader.java#L535-L545).
- Expected behavior: forwarded exports should have a stable external address-space representation and a relation to the forwarder target, without being treated as local executable code.
- Actual behavior: the forwarder is a string-only record; no external block, external symbol, or reference target exists in the native program model.
- Impact: analyzers cannot create or resolve references to forwarded exports and the observable symbol table differs for PE DLLs using forwarders.
- Reproduction or failure scenario: load a DLL whose export directory contains a forwarded export; `exported_symbols()` contains it, but `memory_regions()` and analyzer state contain no corresponding external location.
- Root cause: the loader boundary models preferred-image PE memory only and omits Ghidra's external address space.
- Recommended fix: add an explicit external address-space/export-forwarder model shared by the loader and analyzer context, with stable identity and reference semantics distinct from local VA addresses.
- Regression risks: external addresses must never collide with preferred-image VAs or be passed to Sleigh as local code; add forwarded and non-forwarded export fixtures.
- Relevant tests or validation: no current loader/analyzer integration test asserts the external-block side effect for forwarded exports.

### Low

No findings.

## Verified Strengths

- [x] RVA, VA, and file-offset translations use checked arithmetic and return categorized errors.
- [x] Directory parsing preserves partial-status diagnostics in non-strict mode.
- [x] Import, delay-import, export, relocation, debug, exception, TLS, resource, certificate, and CLR records are exposed as immutable value views.
- [x] `GHIDRA_INSTALL_DIR` was verified as a valid installed Ghidra path.

## Reviewed Areas With No Findings

- [x] Checked image-base overflow handling in `rva_to_va` and `va_to_rva`.
- [x] Complete-range checks in `find_memory_region` and `read_memory` for the ranges represented by the current region metadata.
- [x] Strict versus partial parse-status plumbing.

## Validation

- [x] `git diff --check` completed without whitespace errors.
- [x] `ctest --test-dir NEW/build -N` listed `pe_loader_tests` and the dependent analyzer tests.
- [x] Runtime build/test execution completed in the authorized follow-up: `NEW\build.bat all`, 49/49 tests passed.

## Unresolved Questions And Residual Risks

- [ ] The exact native policy for representing an external address space must be coordinated with the shared analyzer context and reference model.
- [ ] Malformed but accepted PE section alignment cases need dedicated fixtures before changing mapping semantics.

## Follow-Up Decision

- [x] Structured symbols/relocations and PE32 width mapping were implemented; exhaustive machine mapping remains a residual risk.
- [ ] Remaining PE follow-up: explicit external-space policy and malformed section-alignment fixtures.
