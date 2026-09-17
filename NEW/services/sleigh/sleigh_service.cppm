export module recode.service.sleigh;

import std;
import recode.core;
import recode.runtime.workers.pool;
import sleigh_runtime;

export namespace recode::services::sleigh {

namespace core = recode::core;
namespace runtime = recode::runtime;

/// Adapts the stateful Sleigh decoder into a mutex-protected canonical decoder service.
class SleighService final : public core::contracts::IPCodeDecoder {
public:
    /// Loads one compiled SLA and associates it with the shared runtime pool.
    static core::Result<std::shared_ptr<SleighService>> open(std::filesystem::path path,
                                                             std::shared_ptr<runtime::workers::WorkerPool> pool) {
        try {
            return std::shared_ptr<SleighService>(new SleighService(std::move(path), std::move(pool)));
        } catch (const std::exception& error) {
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::resource_unavailable, error.what(),
                                  "Verify that the compiled SLA resource exists and matches the language."));
        }
    }

    /// Decodes one bounded instruction while serializing access to native state.
    [[nodiscard]] core::Result<core::DecodedInstruction>
    decode(const core::contracts::DecodeRequest& request) const override {
        std::scoped_lock lock(mutex_);
        try {
            sleigh_runtime::ProcessorContext context;
            for (const auto& [name, value] : request.context.values)
                context.values.push_back({name, value});
            auto decoded = decoder_->decode(request.address.offset, request.bytes.view(), context);
            if (!decoded)
                return std::unexpected(core::Error::make(core::DiagnosticCode::parse_failure, decoded.error().message));
            return *decoded;
        } catch (const std::exception& error) {
            return std::unexpected(core::Error::make(core::DiagnosticCode::parse_failure,
                                                     std::string("Sleigh decode threw: ") + error.what()));
        } catch (...) {
            return std::unexpected(core::Error::make(core::DiagnosticCode::parse_failure,
                                                     "Sleigh decode threw an unknown native exception"));
        }
    }

    /// Queues deterministic address-ordered decoding through the shared worker pool.
    [[nodiscard]] core::contracts::Task<core::Result<core::contracts::DecodeBatchResult>>
    decode_batch(const core::contracts::DecodeBatchRequest& request,
                 core::contracts::OperationContext context) const override {
        auto submitted = pool_->submit(
            context.project, core::contracts::WorkPriority::analysis,
            [this, request](core::contracts::CancellationToken token) {
                core::contracts::DecodeBatchResult result;
                result.instructions.reserve(request.requests.size());
                for (const auto& item : request.requests) {
                    if (token.stop_requested())
                        return core::Result<core::contracts::DecodeBatchResult>{std::unexpected(
                            core::Error::make(core::DiagnosticCode::cancelled, "Sleigh batch decode was cancelled"))};
                    auto decoded = decode(item);
                    if (!decoded)
                        return core::Result<core::contracts::DecodeBatchResult>{std::unexpected(decoded.error())};
                    result.instructions.push_back(std::move(*decoded));
                }
                std::ranges::sort(result.instructions, {}, [](const auto& instruction) { return instruction.address; });
                return core::Result<core::contracts::DecodeBatchResult>{std::move(result)};
            });
        if (!submitted)
            throw std::runtime_error(submitted.error().message);
        return std::move(*submitted);
    }

    /// Returns the compiled language resource path used for diagnostics and identity.
    [[nodiscard]] const std::filesystem::path& specification_path() const noexcept {
        return specification_path_;
    }

private:
    /// Constructs a decoder after the factory has validated the resource.
    SleighService(std::filesystem::path path, std::shared_ptr<runtime::workers::WorkerPool> pool)
        : specification_path_(std::move(path)),
          decoder_(std::make_unique<sleigh_runtime::Decoder>(specification_path_)), pool_(std::move(pool)) {}

    std::filesystem::path specification_path_;
    std::unique_ptr<sleigh_runtime::Decoder> decoder_;
    std::shared_ptr<runtime::workers::WorkerPool> pool_;
    mutable std::mutex mutex_;
};

} // namespace recode::services::sleigh
