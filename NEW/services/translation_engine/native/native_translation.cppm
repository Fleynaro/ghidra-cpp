export module ghidra.service.translation_engine.native;

import std;
export import ghidra.core.architecture;

export namespace ghidra::services::translation_engine {

/// Re-exports the canonical architecture value instead of defining a translation-engine duplicate.
using NativeTranslationEngine = ghidra::core::ArchitectureDescription;

} // namespace ghidra::services::translation_engine
