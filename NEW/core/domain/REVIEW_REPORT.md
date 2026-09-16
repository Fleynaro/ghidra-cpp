# Core Domain Review Report

## Review Metadata

- [x] **Scope:** `NEW/core/domain` as the canonical comparison target for a read-only audit of `NEW/services` and related runtime service adapters.
- [x] **Reviewed sources:** current domain value models, aggregate exports, and the service-side model/conversion references listed below.
- [x] **Date:** 2026-09-16.
- [x] **Reviewer:** Kilo, independent read-only pass.
- [x] **Assumption:** Domain values are intentionally identity-bearing and revision/project neutral; provider/parser algorithm records may remain local when their lifecycle or representation is materially different.

## Review Status

- [x] Scope confirmation.
- [x] Domain source inspection.
- [x] Service duplicate-model and conversion inspection.
- [x] Test/build validation status recorded.
- [x] No domain or service implementation fixes applied by this review.

## Findings: Critical

### No findings

## Findings: High

### No findings

## Findings: Medium

### No findings

## Findings: Low

### No findings

## Verified Strengths

- [x] `address.cppm`, `address_range.cppm`, `decoded_instruction.cppm`, `instruction.cppm`, `function.cppm`, `reference.cppm`, `data_object.cppm`, `analysis_fact.cppm`, and `architecture.cppm` provide explicit identity/address-space-aware canonical values for service integration.
- [x] `decoded_instruction.cppm:49-75` is an appropriate transient decoder boundary; `instruction.cppm:18-37` is the persistent listing snapshot, so decoder promotion can remain explicit rather than conflating lifecycles.
- [x] `sleigh_runtime.cppm:10-21` and `decompiler/src/decompiler.cppm:8-21` currently re-export several canonical transient values instead of defining duplicate p-code/operand models.
- [x] Domain models retain provenance, revision, entity identity, and address-space information needed to prevent service-local numeric-offset collisions.

## Reviewed Areas With No Findings

- [x] Domain aggregate exports and module naming.
- [x] Canonical address, range, instruction, flow, p-code, function, symbol, variable, data, fact, and architecture value declarations.
- [x] Runtime projection storage of canonical values in `../../runtime/projections/software_model_projection.cppm:77-153,193-267`.

## Validation Results

- [x] Read-only source inspection with path/symbol cross-reference.
- [x] Existing repository validation recorded by the aggregate service review: `ctest --test-dir NEW/build --output-on-failure` reported 49/49 passed before concurrent worktree edits.
- [ ] No new build, test, sanitizer, or alternate-architecture run was performed for this read-only audit.

## Unresolved Questions And Residual Risks

- [ ] Decide whether `core::DecodedInstruction` should become the only transient decoder return type before deleting compatibility aliases.
- [ ] Define whether provider metadata extensions belong in `core/contracts/decompiler` or remain native frontend-private.
- [ ] Preserve entity IDs, address spaces, provenance, and revision evidence when migrating legacy analyzer rows.

## Follow-Up Decision

- [ ] No domain findings require remediation in this audit; service-side duplicate-model findings are tracked in [`../../services/REVIEW_REPORT.md`](../../services/REVIEW_REPORT.md).
- [x] Follow-up fixes added `materialize_decoded_instruction()` and address-format round-trip coverage; full build/test validation completed.
