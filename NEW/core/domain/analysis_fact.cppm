export module recode.core.analysis_fact;

import std;
import recode.core.address;
import recode.core.identifiers;
import recode.core.storage_location;

export namespace recode::core {

/// Carries provenance and confidence shared by derived analysis facts.
struct AnalysisEvidence {
    std::string source_service;
    double confidence{};
    Revision revision;
    std::vector<EntityId> evidence_entities;
    std::string explanation;
};

/// Records a constant propagated from one instruction/storage location.
struct ConstantFact {
    EntityId id;
    Address instruction;
    StorageLocation location;
    std::uint64_t value{};
    AnalysisEvidence evidence;
};

/// Represents one structured Function ID match fact.
struct FunctionIdMatch {
    EntityId id;
    EntityId function;
    std::string library;
    std::string name;
    double score{};
    AnalysisEvidence evidence;
};

/// Represents a recovered switch/jump-table fact.
struct SwitchFact {
    EntityId id;
    Address branch;
    std::vector<Address> targets;
    AnalysisEvidence evidence;
};

/// Represents a recovered stack variable fact.
struct StackVariable {
    EntityId id;
    EntityId function;
    std::int64_t offset{};
    std::uint32_t size{};
    std::string name;
    AnalysisEvidence evidence;
};

} // namespace recode::core
