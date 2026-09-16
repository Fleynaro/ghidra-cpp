# Test Coverage Review: services/tests

## Review Metadata

- [x] **Scope:** services/tests, its CMake registration, test sources, fixtures, and the architecture commits 778c5d87ad and 3c123d1fda.
- [x] **Date:** 2026-09-16.
- [x] **Reviewer:** Kilo, independent test-coverage review.
- [x] **Assumptions:** Existing green tests are regression coverage, not proof of complete architecture behavior. Generated `NEW/build` directories are excluded.

## Inventory

- [x] Existing files: CMakeLists.txt, README.md, service_tests.cppm.
- [x] CMake registration and nearby target ownership inspected.

## Findings: Critical

### No findings

No missing test was classified as critical in this directory. Runtime lifetime gaps are tracked in the root report.

## Findings: High

### TEST-SERVICES_TESTS-HIGH-001: Service adapter tests do not assert structured contract fields

- [ ] **Remediation status:** Open.
- **Affected component:** services/tests.
- **Source references:** service_tests.cppm. CMake registration is in `../CMakeLists.txt` where present.
- **Technical evidence:** PE regions/architecture and one x86 ret decode are tested, but structured imports/exports/relocations, operand mappings, malformed FID input, cancellation, alternate architectures, and service lifetime are not.
- **Expected behavior:** The suite proves the component contract through valid, invalid, failure, and runtime-composed cases.
- **Actual behavior:** PE regions/architecture and one x86 ret decode are tested, but structured imports/exports/relocations, operand mappings, malformed FID input, cancellation, alternate architectures, and service lifetime are not.
- **Impact:** Important regressions can pass the focused suite.
- **Failure scenario:** A future change breaks the listed contract while preserving current happy-path fixtures.
- **Root cause:** Coverage was migrated around the successful path without the required failure/contract scenarios.
- **Recommended fix:** Add the required tests below to a deterministic CTest target.
- **Regression risks:** New fixtures and fault injection must remain deterministic and CI-safe.
- **Relevant validation:** Full CTest currently passes 49/49, but does not exercise this gap.

#### Required tests

- [ ] PE symbols/relocations/directories/invalid options.
- [ ] Sleigh address/register operands, ARM/32-bit, invalid SLA, cancellation.
- [ ] Cross-service decoder failure and destruction tests.

## Findings: Medium

### No medium-severity test gap identified

## Findings: Low

### No low-severity test gap identified

## Verified Strengths

- [x] Existing test sources and fixtures remain available in this directory.
- [x] CMake/CTest ownership was inspected.
- [x] Full CTest currently reports 49/49 passing tests.

## Reviewed Areas With No Findings

- [x] Existing successful-path assertions.
- [x] Existing fixture path ownership.
- [x] Existing CMake/CTest registration where present.

## Validation Results

- [x] Test source and CMake registration inspected.
- [x] `ctest --test-dir NEW/build --output-on-failure`: 49/49 passed.
- [x] `git diff --check`: passed for implementation commits.
- [ ] No tests were added by this review.

## Unresolved Questions And Residual Risks

- [ ] Required tests above must be assigned to deterministic CTest targets and fixture owners.
- [ ] Cross-directory integration gaps may require shared fake services and fault-injection helpers.

## Follow-Up Decision

- [ ] Implement the listed missing tests before treating this directory coverage as complete.
- [ ] No source or test implementation changes were authorized.