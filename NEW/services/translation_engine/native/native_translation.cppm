export module recode.service.translation_engine.native;

import std;
export import recode.core.architecture;

export namespace recode::services::translation_engine {

/// Re-exports the canonical architecture value instead of defining a translation-engine duplicate.
using NativeTranslationEngine = recode::core::ArchitectureDescription;

} // namespace recode::services::translation_engine
