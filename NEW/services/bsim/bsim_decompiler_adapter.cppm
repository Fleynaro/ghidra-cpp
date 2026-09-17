export module recode.service.bsim.decompiler_adapter;

import std;
import decompiler;
import recode.core.contracts.bsim;
import recode.core.function;
import recode.service.bsim;

// This is a boundary adapter only. Signature generation remains delegated to
// Ghidra's existing port in NEW/services/decompiler/src/signature.cppm and the
// compact fields originate from Ghidra's SignatureResult.java.

export namespace recode::services::bsim {

/// Converts the native Decompiler's value-owned signature result into BSim's contract.
[[nodiscard]] inline recode::core::Result<recode::core::contracts::FunctionSimilarityResult>
analyze_decompiler(const BsimService& service, const recode::core::FunctionSnapshot& snapshot,
                   const recode::decompiler::DecompilationResult& decompilation) {
    recode::core::contracts::FunctionSimilarityFeatures features;
    features.function = snapshot.key;
    features.hashes = decompilation.signature_features;
    features.direct_call_addresses = decompilation.signature_call_addresses;
    features.has_unimplemented = decompilation.signature_has_unimplemented;
    features.has_bad_data = decompilation.signature_has_bad_data;
    features.overall_hash = decompilation.signature_overall_hash;
    return service.analyze(features);
}

} // namespace recode::services::bsim
