# Ghidra Port Evidence

- Original analyzer family: [`Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ConstantPropagationAnalyzer.java`](../../../../Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/ConstantPropagationAnalyzer.java).
- Original x86 script: [`Ghidra/Features/Base/ghidra_scripts/PropagateX86ConstantReferences.java`](../../../../Ghidra/Features/Base/ghidra_scripts/PropagateX86ConstantReferences.java).
- Native source: [`src/x86_constant_reference.cppm`](src/x86_constant_reference.cppm).
- Native tests: [`tests/x86_constant_reference_tests.cppm`](tests/x86_constant_reference_tests.cppm).

The port preserves x86-only eligibility, LEA recognition, propagated-register preference,
`> 0x1000` pointer filtering, VA/RVA mapping through the existing PE loader, DATA reference kind,
operand index, and duplicate suppression in `AnalysisContext::add_reference`.

The Java script also performs symbolic switch-table recovery, speculative branch exploration,
address-table creation, labels, and mnemonic references. The native public model has no equivalent
address-table repair or namespace API; those behaviors are intentionally not guessed or represented
as ordinary data references.
