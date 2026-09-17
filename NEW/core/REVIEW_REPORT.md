# Core Module Review Report

## Review Metadata

- [x] **Scope:** exactly `HEAD~2..HEAD` (`778c5d87ad` and `3c123d1fda`), limited to `core` and the core-facing portions of the migration.
- [x] **Date:** 2026-09-16.
- [x] **Reviewer:** Kilo, independent read-only pass.
- [x] **Assumption:** runtime/service implementation findings are recorded in sibling reports; this report covers the declarations and invariants exposed by core.

## Review Status

- [x] Scope confirmation.
- [x] Core source, CMake module registration, and tests inspected.
- [x] Contract boundary inspection completed.
- [x] Validation status recorded.
- [x] No implementation fixes applied.

## Findings: Critical

### No findings

No confirmed critical defect was found in the core declarations themselves. The critical task-lifetime defect is in service/runtime ownership and is tracked in [`../REVIEW_REPORT.md`](../REVIEW_REPORT.md#critical-001).

## Findings: High

### No findings

## Findings: Medium

### No findings

The PE/Sleigh field-loss finding is in the adapters, not in the core DTO declarations: [`../REVIEW_REPORT.md`](../REVIEW_REPORT.md#medium-005).

## Findings: Low

### No findings

## Verified Strengths

- [x] `core/domain/address.cppm` preserves address-space identity and checked arithmetic.
- [x] `core/contracts/*.cppm` separates immutable values/providers from runtime ownership.
- [x] `core/domain/pcode.cppm`, `operand.cppm`, and `architecture.cppm` provide explicit fields for p-code, typed operands, and architecture metadata.
- [x] Core CMake registration includes domain, contract, event, and focused test modules.

## Reviewed Areas With No Findings

- [x] Identifier, address, range, bytes, p-code, instruction, and architecture value declarations.
- [x] `IPCodeDecoder`, `IPELoader`, `IDecompiler`, `IAnalyzer`, and operation/task contract declarations.
- [x] Core event draft/envelope declarations and event helper payloads.
- [x] Core CMake target and module registration.

## Validation Results

- [x] Exact two-commit diff and current core source were inspected read-only.
- [x] Existing core test registration was inspected.
- [x] `ctest --test-dir build --output-on-failure`: 49/49 passed.
- [x] `git diff HEAD~2..HEAD --check`: passed.
- [ ] Sanitizer, ABI, and module-interface stress validation were not executed.

## Unresolved Questions And Residual Risks

- [ ] Service adapters must populate all fields promised by the core DTOs; see the cross-module report.
- [ ] Operation cancellation semantics must remain consistent with `OperationContext` after worker-pool remediation.

## Follow-Up Decision

- [x] No core-only remediation is proposed.
- [ ] Cross-module remediation remains open in [`../REVIEW_REPORT.md`](../REVIEW_REPORT.md).
