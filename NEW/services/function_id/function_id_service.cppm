export module recode.service.function_id;

import std;
import recode.core;
import recode.runtime.workers.pool;
import function_id;

export namespace recode::services::function_id {

namespace core = recode::core;
namespace runtime = recode::runtime;

/// Adapts one immutable packed Function ID database to the core database contract.
class FunctionIdDatabaseService final : public core::contracts::IFunctionIdDatabase {
public:
    /// Opens a packed database and translates parser errors to core errors.
    static core::Result<std::shared_ptr<FunctionIdDatabaseService>> open(std::filesystem::path path) {
        auto database = fid::Database::open(path);
        if (!database)
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::resource_unavailable, database.error().message));
        return std::shared_ptr<FunctionIdDatabaseService>(
            new FunctionIdDatabaseService(std::move(path), std::move(*database)));
    }

    /// Returns the immutable database path identity.
    [[nodiscard]] std::string identity() const override {
        return path_.generic_string();
    }

    /// Returns candidates sharing a full hash while keeping database objects private.
    [[nodiscard]] core::Result<std::vector<core::FunctionIdCandidate>>
    query(const core::FunctionHashFamily& hashes) const override {
        std::uint64_t full_hash{};
        try {
            full_hash = std::stoull(hashes.full_hash, nullptr, 16);
        } catch (const std::exception& error) {
            return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                     std::string("Function ID full hash is invalid: ") + error.what()));
        }
        const auto records = database_.find_full_hash(full_hash);
        std::vector<core::FunctionIdCandidate> result;
        result.reserve(records.size());
        for (const auto& record : records)
            result.push_back(core::FunctionIdCandidate{"", record.name, 0.0, "packed", false});
        return result;
    }

private:
    /// Stores validated immutable database state.
    FunctionIdDatabaseService(std::filesystem::path path, fid::Database database)
        : path_(std::move(path)), database_(std::move(database)) {}

    std::filesystem::path path_;
    fid::Database database_;
};

/// Runs the original Function ID hasher/scorer against a revision-stamped query.
class FunctionIdService final : public core::contracts::IFunctionIdMatcher {
public:
    /// Constructs a service over a database, immutable project query, and shared worker pool.
    FunctionIdService(std::shared_ptr<const FunctionIdDatabaseService> database,
                      std::shared_ptr<const core::contracts::IProjectQuery> query,
                      std::shared_ptr<runtime::workers::WorkerPool> pool)
        : database_(std::move(database)), query_(std::move(query)), pool_(std::move(pool)) {}

    /// Queues relation-aware identification on the shared pool.
    [[nodiscard]] core::contracts::Task<core::Result<core::FunctionIdResult>>
    identify(core::FunctionSnapshot function, core::FunctionIdOptions options,
             core::contracts::OperationContext context) override {
        auto submitted =
            pool_->submit(context.project, core::contracts::WorkPriority::analysis,
                          [this, function = std::move(function),
                           options = std::move(options)](core::contracts::CancellationToken token) {
                              if (token.stop_requested())
                                  return core::Result<core::FunctionIdResult>{std::unexpected(core::Error::make(
                                      core::DiagnosticCode::cancelled, "Function ID identification was cancelled"))};
                              return identify_now(function, options);
                          });
        if (!submitted)
            throw std::runtime_error(submitted.error().message);
        return std::move(*submitted);
    }

private:
    /// Hashes query instructions and translates immutable packed candidates.
    [[nodiscard]] core::Result<core::FunctionIdResult> identify_now(const core::FunctionSnapshot& function,
                                                                    const core::FunctionIdOptions& options) const {
        std::vector<fid::Instruction> instructions;
        for (const auto& instruction : query_->instructions()) {
            if (instruction.key.address.offset < function.key.entry.offset)
                continue;
            if (!function.body.contains(instruction.key.address))
                continue;
            fid::Instruction candidate;
            candidate.bytes = instruction.bytes.values();
            candidate.instruction_mask = instruction.instruction_mask;
            candidate.is_call = instruction.flow.kind == core::FlowKind::call ||
                                instruction.flow.kind == core::FlowKind::conditional_call;
            for (const auto& operand : instruction.operands) {
                std::vector<fid::OperandObject> objects;
                for (const auto& object : operand.objects)
                    objects.push_back(fid::OperandObject{static_cast<fid::OperandObjectKind>(object.kind), object.value,
                                                         object.whole_scalar, object.address_scalar, object.relocated});
                candidate.operands.push_back(std::move(objects));
            }
            instructions.push_back(std::move(candidate));
        }
        auto hash = fid::Hasher::hash(instructions);
        if (!hash)
            return std::unexpected(core::Error::make(core::DiagnosticCode::unsupported, hash.error().message));
        core::FunctionIdResult result;
        result.function = function.key;
        result.hashes.full_hash = to_hex(hash->full_hash);
        result.hashes.specific_hash = to_hex(hash->specific_hash);
        auto identified = database_->query(result.hashes);
        if (!identified)
            return std::unexpected(identified.error());
        result.candidates = std::move(*identified);
        if (!result.candidates.empty()) {
            result.selected = result.candidates.front();
            result.label_threshold_met = result.selected->score >= options.label_threshold;
        }
        result.evidence = core::AnalysisEvidence{"function_id",
                                                 result.selected ? result.selected->score : 0.0,
                                                 query_->current_revision(),
                                                 {},
                                                 "Original packed Function ID scoring"};
        return result;
    }

    /// Formats a hash as the stable lowercase hexadecimal identity used by events.
    [[nodiscard]] static std::string to_hex(std::uint64_t value) {
        std::ostringstream output;
        output << std::hex << value;
        return output.str();
    }

    std::filesystem::path database_path_;
    std::shared_ptr<const FunctionIdDatabaseService> database_;
    std::shared_ptr<const core::contracts::IProjectQuery> query_;
    std::shared_ptr<runtime::workers::WorkerPool> pool_;
};

} // namespace recode::services::function_id
