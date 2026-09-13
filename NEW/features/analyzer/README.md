# Function Analyzer Test Data

This directory contains integration-test data for future C++23 ports of Ghidra
analyzers. It deliberately contains no analyzer implementation or build target
in the C++ project.

## Navigation

- [`test_data/embedded_media/README.md`](test_data/embedded_media/README.md) describes the real embedded-media PE.
- [`test_data/external_entry_references/README.md`](test_data/external_entry_references/README.md) describes the PE-export entry-reference fixture.
- [`test_data/function_id/README.md`](test_data/function_id/README.md) describes the real generated FID database fixture.
- [`test_data/function_start_search/README.md`](test_data/function_start_search/README.md) describes the x64 pattern-search fixture.
- [`test_data/non_returning_functions_discovered/README.md`](test_data/non_returning_functions_discovered/README.md) describes the thresholded INT3 evidence fixture.
- [`test_data/non_returning_functions_known/README.md`](test_data/non_returning_functions_known/README.md) describes the PE known-name fixture.
- [`test_data/subroutine_references/README.md`](test_data/subroutine_references/README.md) describes the existing Subroutine References fixture.
- [`../README.md`](../README.md) describes the surrounding C++23 feature collection.

## Complete Fixture Index

- [`test_data/aggressive_instruction_finder/`](test_data/aggressive_instruction_finder/)
- [`test_data/apply_data_archives/`](test_data/apply_data_archives/)
- [`test_data/ascii_strings/`](test_data/ascii_strings/)
- [`test_data/call_convention_id/`](test_data/call_convention_id/)
- [`test_data/call_fixup_installer/`](test_data/call_fixup_installer/)
- [`test_data/condense_filler_bytes/`](test_data/condense_filler_bytes/)
- [`test_data/create_address_tables/`](test_data/create_address_tables/)
- [`test_data/data_reference/`](test_data/data_reference/)
- [`test_data/decompiler_parameter_id/`](test_data/decompiler_parameter_id/)
- [`test_data/decompiler_switch_analysis/`](test_data/decompiler_switch_analysis/)
- [`test_data/demangler_microsoft/`](test_data/demangler_microsoft/)
- [`test_data/disassemble_entry_points/`](test_data/disassemble_entry_points/)
- [`test_data/embedded_media/`](test_data/embedded_media/)
- [`test_data/external_entry_references/`](test_data/external_entry_references/)
- [`test_data/function_id/`](test_data/function_id/)
- [`test_data/function_start_search/`](test_data/function_start_search/)
- [`test_data/non_returning_functions_discovered/`](test_data/non_returning_functions_discovered/)
- [`test_data/non_returning_functions_known/`](test_data/non_returning_functions_known/)
- [`test_data/pdb_msdia/`](test_data/pdb_msdia/)
- [`test_data/pdb_universal/`](test_data/pdb_universal/)
- [`test_data/reference/`](test_data/reference/)
- [`test_data/scalar_operand_references/`](test_data/scalar_operand_references/)
- [`test_data/shared_return_calls/`](test_data/shared_return_calls/)
- [`test_data/stack/`](test_data/stack/)
- [`test_data/variadic_function_signature_override/`](test_data/variadic_function_signature_override/)
- [`test_data/windows_pe_x86_propagate_external_parameters/`](test_data/windows_pe_x86_propagate_external_parameters/)
- [`test_data/windows_resource_reference/`](test_data/windows_resource_reference/)
- [`test_data/x86_constant_reference/`](test_data/x86_constant_reference/)

The x86-specific fixtures intentionally use 32-bit MSVC because their referenced analyzers
inspect x86 calling conventions or instruction encodings. PDB, resource, Function ID, and
data-archive fixtures document their auxiliary artifacts and environment requirements in
their individual READMEs.
