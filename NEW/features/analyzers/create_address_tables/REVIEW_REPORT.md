# Strict Audit Report

## Scope
- [x] Reviewed `src/create_address_tables.cppm`, tests, fixture/report, CMake, and build wrapper.
- [x] Compared against `AddressTableAnalyzer.java` and `AddressTable.java`.

## Critical
- [ ] **TABLE-CRITICAL-001:** The native scanner is not a 1:1 port of the approximately 1,500-line `AddressTable` algorithm; shifted pointers, secondary indexes, threshold calculation, collision rules, labels, and incremental scheduling are absent.

## High
- [ ] **TABLE-HIGH-001:** Java materializes one pointer data unit per entry; the native test previously asserted an aggregate object. The native model/test were corrected to publish pointer data per entry, but the full table algorithm remains absent.
- [ ] **TABLE-HIGH-002:** Default enablement and dynamic minimum-table threshold are not represented by the shared lifecycle model.

## Medium
- [ ] **TABLE-MEDIUM-001:** Relocation guidance, UTF-16 rejection, offcuts, internal references, invalid entries, and table splitting lack adversarial tests.

## Low
No findings.

## Validation
- [x] Pointer-per-entry materialization is covered by the fixture test.
- [x] Overflow and collision preflight are covered by shared regression tests.
- [ ] Full Java AddressTable parity is not established.

## Follow-up
- [ ] Build an autonomous AddressTable subsystem before adding analyzer-local heuristics.
