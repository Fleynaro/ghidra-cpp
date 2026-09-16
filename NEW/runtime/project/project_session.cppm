export module ghidra.runtime.project.session;

import std;
import ghidra.core;
import ghidra.runtime.event_bus;
import ghidra.runtime.analysis.registry;
import ghidra.runtime.analysis.scheduler;
import ghidra.runtime.event_store.log;
import ghidra.runtime.projections.coordinator;
import ghidra.runtime.projections.software_model;
import ghidra.runtime.storage.projection;
import ghidra.runtime.workers.pool;
import ghidra.service.decompiler;
import ghidra.service.pe_loader;
import ghidra.service.sleigh;
import ghidra.service.analyzers.entry_materialization;
import ghidra.runtime.project.config;
import ghidra.runtime.project.state;

export namespace ghidra::runtime::project {

namespace core = ghidra::core;
namespace pe_service = ghidra::services::pe_loader;
namespace sleigh_service = ghidra::services::sleigh;
namespace decompiler_service = ghidra::services::decompiler;

/// Owns one project's lifecycle, services, authoritative history, and current projection.
class ProjectSession final : public core::contracts::ICommandHandler,
                             public std::enable_shared_from_this<ProjectSession> {
public:
    /// Opens/recover one project directory and rebuilds its current projection from history.
    static core::Result<std::shared_ptr<ProjectSession>>
    open(ProjectConfig config, std::shared_ptr<workers::WorkerPool> workers, std::shared_ptr<event_bus::EventBus> bus) {
        std::error_code error;
        std::filesystem::create_directories(config.directory, error);
        if (error)
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::io_failure, "Unable to create the project directory"));
        auto log = event_store::AppendOnlyLog::open(config.directory / "events.log");
        if (!log)
            return std::unexpected(log.error());
        auto projection_store =
            std::make_shared<storage::SqliteProjectionStore>(config.directory / "projection.sqlite");
        if (const auto opened = projection_store->open(config.id); !opened)
            return std::unexpected(opened.error());
        auto projection = std::make_shared<projections::SoftwareModelProjection>(config.id);
        auto stream = (*log)->read(config.id, core::Revision{1});
        if (!stream)
            return std::unexpected(stream.error());
        if (const auto rebuilt = projection->rebuild(stream->events); !rebuilt)
            return std::unexpected(rebuilt.error());
        auto session = std::shared_ptr<ProjectSession>(
            new ProjectSession(std::move(config), std::move(workers), std::move(bus), std::move(*log),
                               std::move(projection_store), std::move(projection)));
        if (const auto registered = session->analyzer_registry_->register_analyzer(
                std::make_shared<ghidra::services::analyzers::EntryMaterializationAnalyzer>());
            !registered)
            return std::unexpected(registered.error());
        session->state_.status = ProjectStatus::open;
        session->state_.revision = session->projection_->checkpoint();
        if (session->state_.revision.value == 0) {
            const auto correlation = core::CorrelationId{core::make_identifier("open", session->next_operation_++)};
            const auto created = core::events::project_created(session->config_.id, correlation);
            if (const auto committed = session->commit(std::span{&created, 1}, std::nullopt); !committed)
                return std::unexpected(committed.error());
        }
        return session;
    }

    /// Closes services and makes all later mutation requests fail.
    void close() noexcept {
        std::scoped_lock lock(mutex_);
        if (state_.status == ProjectStatus::closed)
            return;
        state_.status = ProjectStatus::closing;
        if (log_)
            log_->close();
        state_.status = ProjectStatus::closed;
    }

    /// Returns the stable project identity.
    [[nodiscard]] const core::ProjectId& id() const noexcept {
        return config_.id;
    }

    /// Returns the current lifecycle snapshot.
    [[nodiscard]] ProjectState state() const {
        std::scoped_lock lock(mutex_);
        return state_;
    }

    /// Returns the current revision-stamped project query.
    [[nodiscard]] std::shared_ptr<const core::contracts::IProjectQuery> query() const noexcept {
        return projection_;
    }

    /// Loads the primary PE, materializes regions, decodes the entry body, and commits all state changes.
    [[nodiscard]] core::Result<LoadSummary> load_primary_binary() {
        std::scoped_lock lock(mutex_);
        if (state_.status == ProjectStatus::closed)
            return std::unexpected(core::Error::make(core::DiagnosticCode::project_closed, "Project is closed"));
        state_.status = ProjectStatus::loading;
        const auto correlation = core::CorrelationId{core::make_identifier("load", next_operation_++)};
        auto loaded = loader_.load(config_.primary_artifact, config_.load_options);
        if (!loaded) {
            state_.status = ProjectStatus::failed;
            return std::unexpected(loaded.error());
        }
        auto image = loader_.open(config_.primary_artifact.locator, config_.load_options);
        if (!image) {
            state_.status = ProjectStatus::failed;
            return std::unexpected(image.error());
        }
        image_ = std::move(*image);
        architecture_ = std::make_shared<core::ArchitectureDescription>(loaded->architecture);
        std::vector<core::events::EventDraft> drafts;
        drafts.push_back(core::events::project_inputs_changed(config_.id, config_.primary_artifact, correlation));
        for (const auto& region : image_->regions())
            drafts.push_back(core::events::memory_state_changed(config_.id, region, correlation));
        if (const auto committed = commit(drafts, std::nullopt); !committed) {
            state_.status = ProjectStatus::failed;
            return std::unexpected(committed.error());
        }
        if (config_.sleigh_specification.empty())
            config_.sleigh_specification = std::filesystem::path(SLEIGH_RUNTIME_SPECIFICATION_DIR) / "x86-64.sla";
        auto decoder = sleigh_service::SleighService::open(config_.sleigh_specification, workers_);
        if (!decoder)
            return std::unexpected(decoder.error());
        decoder_ = std::move(*decoder);
        auto entry = image_->entry_point();
        if (!entry)
            return std::unexpected(entry.error());
        auto body = decode_entry(*entry, correlation);
        if (!body) {
            state_.status = ProjectStatus::failed;
            return std::unexpected(body.error());
        }
        if (!body->instructions.empty()) {
            core::FunctionSnapshot function;
            function.key = core::FunctionKey{core::EntityId{core::make_identifier("function", entry->offset)}, *entry};
            function.name = "entry";
            function.analysis_status = "decoded";
            function.instruction_starts.reserve(body->instructions.size());
            std::vector<core::events::EventDraft> listing;
            for (const auto& instruction : body->instructions) {
                listing.push_back(core::events::listing_state_changed(config_.id, instruction, correlation));
                function.instruction_starts.push_back(instruction.key.address.offset);
                const auto end = instruction.key.address.offset + instruction.length - 1;
                function.body.add(
                    core::AddressRange{instruction.key.address, core::Address{instruction.key.address.space, end}});
            }
            listing.push_back(core::events::function_state_changed(config_.id, function, correlation, "sleigh"));
            if (const auto committed = commit(listing, std::nullopt); !committed)
                return std::unexpected(committed.error());
            decompiler_ = std::make_shared<decompiler_service::DecompilerService>(config_.sleigh_specification,
                                                                                  projection_, image_, workers_);
            state_.status = ProjectStatus::ready;
            return LoadSummary{state_.revision, image_->regions().size(), body->instructions.size(), function.key, {}};
        }
        state_.status = ProjectStatus::ready;
        return LoadSummary{state_.revision, image_->regions().size(), 0, std::nullopt, {}};
    }

    /// Starts a deterministic analysis-run lifecycle over the current revision.
    [[nodiscard]] core::Result<AnalysisSummary> analyze() {
        std::scoped_lock lock(mutex_);
        if (state_.status == ProjectStatus::closed)
            return std::unexpected(core::Error::make(core::DiagnosticCode::project_closed, "Project is closed"));
        state_.status = ProjectStatus::analyzing;
        const auto run = core::AnalysisRunId{core::make_identifier("analysis", next_operation_++)};
        const auto correlation = core::CorrelationId{core::make_identifier("analysis-correlation", next_operation_++)};
        const auto started = core::events::analysis_run_state_changed(config_.id, run, "started", correlation);
        if (const auto committed = commit(std::span{&started, 1}, state_.revision); !committed)
            return std::unexpected(committed.error());
        auto history = log_->read(config_.id, core::Revision{1});
        if (!history)
            return std::unexpected(history.error());
        core::contracts::AnalysisSnapshot snapshot;
        snapshot.project = config_.id;
        snapshot.revision = state_.revision;
        snapshot.scope = core::contracts::AnalysisScope::project;
        snapshot.resources = core::ResourceSetIdentity{
            config_.primary_artifact.sha256,
            {core::ResourceIdentity{"primary", config_.primary_artifact.id.value(), config_.primary_artifact.sha256, 0,
                                    "PE", "", true, config_.primary_artifact.locator}}};
        snapshot.architecture = architecture_;
        snapshot.query = projection_;
        snapshot.memory = image_;
        snapshot.decoder = decoder_;
        auto operation = std::make_shared<core::contracts::OperationControl>();
        auto report = scheduler_.run(
            snapshot, core::events::EventBatch{history->events},
            core::contracts::OperationContext{config_.id, state_.revision, operation->cancellation(), operation});
        if (!report)
            return std::unexpected(report.error());
        const auto finished = core::events::analysis_run_state_changed(config_.id, run, "completed", correlation);
        if (const auto committed = commit(std::span{&finished, 1}, state_.revision); !committed)
            return std::unexpected(committed.error());
        state_.status = ProjectStatus::ready;
        return AnalysisSummary{run, report->read_revision, state_.revision, report->executed_analyzers,
                               report->diagnostics};
    }

    /// Queues native decompilation of one current function snapshot.
    [[nodiscard]] core::contracts::Task<core::Result<core::Decompilation>> decompile(core::FunctionKey function) {
        auto current = projection_->function_at(function.entry);
        if (!current || !decompiler_)
            return submit_error<core::Result<core::Decompilation>>(core::Error::make(
                core::DiagnosticCode::resource_unavailable, "No decoded function/decompiler is available"));
        core::contracts::DecompileRequest request;
        request.function = *current;
        request.read_revision = projection_->checkpoint();
        request.providers = core::contracts::ProviderContext{
            std::static_pointer_cast<const core::contracts::IPCodeDecoder>(decoder_), image_, projection_};
        auto operation = std::make_shared<core::contracts::OperationControl>();
        return decompiler_->decompile(std::move(request),
                                      core::contracts::OperationContext{config_.id, projection_->checkpoint(),
                                                                        operation->cancellation(), operation});
    }

    /// Executes the command contract through the same project commit lane used by facade calls.
    [[nodiscard]] core::Result<core::contracts::CommandResponse>
    handle(const core::contracts::CommandRequest& request) override {
        return std::visit([&](const auto& command) { return handle_command(command, request); }, request.payload);
    }

private:
    /// Constructs a session from already-open infrastructure components.
    ProjectSession(ProjectConfig config, std::shared_ptr<workers::WorkerPool> workers,
                   std::shared_ptr<event_bus::EventBus> bus, std::shared_ptr<event_store::AppendOnlyLog> log,
                   std::shared_ptr<storage::SqliteProjectionStore> projection_store,
                   std::shared_ptr<projections::SoftwareModelProjection> projection)
        : config_(std::move(config)), workers_(std::move(workers)), bus_(std::move(bus)), log_(std::move(log)),
          projection_store_(std::move(projection_store)), projection_(std::move(projection)),
          coordinator_(log_, projection_, projection_store_, bus_),
          analyzer_registry_(std::make_shared<analysis::AnalyzerRegistry>()), scheduler_(analyzer_registry_, workers_) {
    }

    /// Appends drafts and applies them through the coordinator while updating lifecycle revision.
    [[nodiscard]] core::Result<core::contracts::AppendResult> commit(std::span<const core::events::EventDraft> drafts,
                                                                     std::optional<core::Revision> expected) {
        auto committed = coordinator_.commit(config_.id, drafts, expected);
        if (committed)
            state_.revision = committed->committed_revision;
        return committed;
    }

    /// Decodes a bounded entry-point window using the canonical Sleigh service.
    [[nodiscard]] core::Result<core::contracts::DecodeBatchResult> decode_entry(core::Address entry,
                                                                                const core::CorrelationId&) const {
        if (!decoder_)
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::resource_unavailable, "Sleigh decoder is not initialized"));
        core::contracts::DecodeBatchResult result;
        auto address = entry;
        for (std::size_t count = 0; count < 64; ++count) {
            auto bytes = image_->read(address, 16);
            if (!bytes)
                break;
            core::contracts::DecodeRequest request{address, std::move(*bytes), {}};
            auto decoded = decoder_->decode(request);
            if (!decoded)
                break;
            result.instructions.push_back(std::move(*decoded));
            const auto& instruction = result.instructions.back();
            if (instruction.flow.terminal || instruction.length == 0)
                break;
            address.offset += instruction.length;
        }
        return result;
    }

    /// Returns a completed task containing a deterministic error without exposing a null task.
    template <class T> [[nodiscard]] core::contracts::Task<T> submit_error(core::Error error) {
        auto submitted = workers_->submit(
            config_.id, core::contracts::WorkPriority::interactive,
            [error = std::move(error)](core::contracts::CancellationToken) { return T{std::unexpected(error)}; });
        if (!submitted)
            throw std::runtime_error(submitted.error().message);
        return std::move(*submitted);
    }

    /// Handles one typed command payload and maps the result into a response.
    template <class Command>
    [[nodiscard]] core::Result<core::contracts::CommandResponse>
    handle_command(const Command&, const core::contracts::CommandRequest& request) {
        return core::contracts::CommandResponse{request.id,
                                                core::contracts::CommandStatus::rejected,
                                                std::nullopt,
                                                {},
                                                std::nullopt,
                                                {core::Diagnostic{core::Severity::warning,
                                                                  core::DiagnosticCode::unsupported,
                                                                  "Command is not enabled by this project session",
                                                                  {},
                                                                  {}}}};
    }

    /// Dispatches the primary-load command through the lifecycle method.
    [[nodiscard]] core::Result<core::contracts::CommandResponse>
    handle_command(const core::contracts::LoadPrimaryBinary&, const core::contracts::CommandRequest& request) {
        auto loaded = load_primary_binary();
        if (!loaded)
            return std::unexpected(loaded.error());
        return core::contracts::CommandResponse{request.id,
                                                core::contracts::CommandStatus::completed,
                                                core::contracts::CommandResult{loaded->revision},
                                                {},
                                                loaded->revision,
                                                {}};
    }

    /// Dispatches the analysis-start command through the lifecycle method.
    [[nodiscard]] core::Result<core::contracts::CommandResponse>
    handle_command(const core::contracts::StartAnalysis&, const core::contracts::CommandRequest& request) {
        auto summary = analyze();
        if (!summary)
            return std::unexpected(summary.error());
        return core::contracts::CommandResponse{request.id,
                                                core::contracts::CommandStatus::completed,
                                                core::contracts::CommandResult{summary->committed_revision},
                                                {},
                                                summary->committed_revision,
                                                {}};
    }

    /// Routes a source-priority function rename through a durable function event.
    [[nodiscard]] core::Result<core::contracts::CommandResponse>
    handle_command(const core::contracts::RenameFunction& command, const core::contracts::CommandRequest& request) {
        auto current = projection_->function_at(command.function.entry);
        if (!current)
            return std::unexpected(
                core::Error::make(core::DiagnosticCode::invalid_argument, "Cannot rename an unknown function"));
        current->name = command.name;
        const auto draft =
            core::events::function_state_changed(config_.id, *current, request.correlation, command.source);
        const auto committed = commit(std::span{&draft, 1}, request.expected_revision);
        if (!committed)
            return std::unexpected(committed.error());
        return core::contracts::CommandResponse{request.id,
                                                core::contracts::CommandStatus::completed,
                                                core::contracts::CommandResult{*current},
                                                {},
                                                committed->committed_revision,
                                                {}};
    }

    /// Routes a user-created function through the same function projection event as analysis output.
    [[nodiscard]] core::Result<core::contracts::CommandResponse>
    handle_command(const core::contracts::CreateFunction& command, const core::contracts::CommandRequest& request) {
        const auto draft =
            core::events::function_state_changed(config_.id, command.function, request.correlation, "user");
        const auto committed = commit(std::span{&draft, 1}, request.expected_revision);
        if (!committed)
            return std::unexpected(committed.error());
        return core::contracts::CommandResponse{request.id,
                                                core::contracts::CommandStatus::completed,
                                                core::contracts::CommandResult{command.function},
                                                {},
                                                committed->committed_revision,
                                                {}};
    }

    /// Runs a selected decompilation synchronously when explicitly requested as an inline command.
    [[nodiscard]] core::Result<core::contracts::CommandResponse>
    handle_command(const core::contracts::DecompileFunction& command, const core::contracts::CommandRequest& request) {
        auto result = decompile(command.function).get();
        if (!result)
            return std::unexpected(result.error());
        return core::contracts::CommandResponse{request.id,
                                                core::contracts::CommandStatus::completed,
                                                core::contracts::CommandResult{std::move(*result)},
                                                {},
                                                projection_->checkpoint(),
                                                {}};
    }

    /// Closes a session only after the command has passed through its owning project handler.
    [[nodiscard]] core::Result<core::contracts::CommandResponse>
    handle_command(const core::contracts::CloseProject&, const core::contracts::CommandRequest& request) {
        close();
        return core::contracts::CommandResponse{request.id,
                                                core::contracts::CommandStatus::completed,
                                                core::contracts::CommandResult{std::monostate{}},
                                                {},
                                                projection_->checkpoint(),
                                                {}};
    }

    ProjectConfig config_;
    mutable std::mutex mutex_;
    ProjectState state_;
    std::shared_ptr<workers::WorkerPool> workers_;
    std::shared_ptr<event_bus::EventBus> bus_;
    std::shared_ptr<event_store::AppendOnlyLog> log_;
    std::shared_ptr<storage::SqliteProjectionStore> projection_store_;
    std::shared_ptr<projections::SoftwareModelProjection> projection_;
    projections::ProjectionCoordinator coordinator_;
    std::shared_ptr<analysis::AnalyzerRegistry> analyzer_registry_;
    analysis::AnalysisScheduler scheduler_;
    pe_service::PeLoaderService loader_;
    std::shared_ptr<pe_service::PeImage> image_;
    std::shared_ptr<sleigh_service::SleighService> decoder_;
    std::shared_ptr<decompiler_service::DecompilerService> decompiler_;
    std::shared_ptr<const core::ArchitectureDescription> architecture_;
    std::uint64_t next_operation_{1};
};

} // namespace ghidra::runtime::project
