export module recode.runtime.projections.software_model;

import std;
import recode.core;
import recode.core.contracts.projection;
import recode.core.contracts.project_query;
import recode.core.events.event;

export namespace recode::runtime::projections {

namespace core = recode::core;

/// Decodes the hexadecimal byte representation persisted by ListingStateChanged.
[[nodiscard]] core::Result<std::vector<std::uint8_t>> decode_hex(std::string_view encoded) {
    if (encoded.size() % 2U != 0)
        return std::unexpected(core::Error::make(core::DiagnosticCode::event_corrupt,
                                                 "Listing event contains an odd-length hexadecimal field"));
    std::vector<std::uint8_t> result;
    result.reserve(encoded.size() / 2U);
    const auto digit = [](char value) -> std::optional<std::uint8_t> {
        if (value >= '0' && value <= '9')
            return static_cast<std::uint8_t>(value - '0');
        if (value >= 'a' && value <= 'f')
            return static_cast<std::uint8_t>(value - 'a' + 10);
        if (value >= 'A' && value <= 'F')
            return static_cast<std::uint8_t>(value - 'A' + 10);
        return std::nullopt;
    };
    for (std::size_t index = 0; index < encoded.size(); index += 2U) {
        const auto high = digit(encoded[index]);
        const auto low = digit(encoded[index + 1U]);
        if (!high || !low)
            return std::unexpected(core::Error::make(core::DiagnosticCode::event_corrupt,
                                                     "Listing event contains invalid hexadecimal bytes"));
        result.push_back(static_cast<std::uint8_t>((*high << 4U) | *low));
    }
    return result;
}

/// Splits a compact decimal field without allocating a second serialization format.
[[nodiscard]] std::vector<std::string_view> split_field(std::string_view value, char separator) {
    std::vector<std::string_view> result;
    while (true) {
        const auto position = value.find(separator);
        result.push_back(value.substr(0, position));
        if (position == std::string_view::npos)
            return result;
        value.remove_prefix(position + 1U);
    }
}

/// Decodes operand object facts required by the Function ID hashing projection.
[[nodiscard]] core::Result<std::vector<core::InstructionOperand>> decode_operands(std::string_view encoded) {
    std::vector<core::InstructionOperand> result;
    if (encoded.empty())
        return result;
    try {
        for (const auto operand_text : split_field(encoded, ';')) {
            const auto first_separator = operand_text.find(':');
            const auto second_separator = first_separator == std::string_view::npos
                                              ? std::string_view::npos
                                              : operand_text.find(':', first_separator + 1U);
            if (first_separator == std::string_view::npos || second_separator == std::string_view::npos)
                return std::unexpected(core::Error::make(core::DiagnosticCode::event_corrupt,
                                                         "Listing event contains an invalid operand field"));
            const auto kind_field = operand_text.substr(0, first_separator);
            const auto value_field = operand_text.substr(first_separator + 1U, second_separator - first_separator - 1U);
            const auto objects_field = operand_text.substr(second_separator + 1U);
            core::InstructionOperand operand;
            operand.kind = static_cast<core::OperandKind>(std::stoul(std::string{kind_field}));
            if (value_field != "_")
                operand.scalar = core::Scalar{std::stoull(std::string{value_field}), 64, false,
                                              operand.kind == core::OperandKind::address, false};
            if (!objects_field.empty())
                for (const auto object_text : split_field(objects_field, ',')) {
                    const auto object = split_field(object_text, ':');
                    if (object.size() != 5)
                        return std::unexpected(core::Error::make(core::DiagnosticCode::event_corrupt,
                                                                 "Listing event contains an invalid operand object"));
                    operand.objects.push_back(core::OperandObject{
                        static_cast<core::OperandObject::Kind>(std::stoul(std::string{object[0]})),
                        std::stoll(std::string{object[1]}), object[2] == "1", object[3] == "1", object[4] == "1"});
                }
            result.push_back(std::move(operand));
        }
    } catch (const std::exception& error) {
        return std::unexpected(
            core::Error::make(core::DiagnosticCode::event_corrupt,
                              std::string("Listing event contains invalid operand numbers: ") + error.what()));
    }
    return result;
}

/// Materializes the current software model as immutable-copy query values.
class SoftwareModelProjection final : public core::contracts::IProjection, public core::contracts::IProjectQuery {
public:
    /// Constructs an empty projection for one project identity.
    explicit SoftwareModelProjection(core::ProjectId project) : project_(std::move(project)) {}

    /// Applies one event idempotently and advances the current revision.
    [[nodiscard]] core::Result<void> apply(const core::events::EventEnvelope& event) override {
        std::scoped_lock lock(mutex_);
        return apply_locked(event);
    }

    /// Applies a batch atomically, restoring the prior copy on a validation failure.
    [[nodiscard]] core::Result<void> apply_batch(std::span<const core::events::EventEnvelope> events) override {
        std::scoped_lock lock(mutex_);
        const auto instructions = instructions_;
        const auto functions = functions_;
        const auto regions = regions_;
        const auto symbols = symbols_;
        const auto references = references_;
        const auto applied = applied_events_;
        const auto revision = revision_;
        for (const auto& event : events) {
            if (const auto result = apply_locked(event); !result) {
                instructions_ = instructions;
                functions_ = functions;
                regions_ = regions;
                symbols_ = symbols;
                references_ = references;
                applied_events_ = applied;
                revision_ = revision;
                return std::unexpected(result.error());
            }
        }
        return {};
    }

    /// Returns the current applied event sequence.
    [[nodiscard]] core::Revision checkpoint() const override {
        std::scoped_lock lock(mutex_);
        return revision_;
    }

    /// Clears all current rows and applies history in sequence order.
    [[nodiscard]] core::Result<void> rebuild(std::span<const core::events::EventEnvelope> events) override {
        std::scoped_lock lock(mutex_);
        instructions_.clear();
        functions_.clear();
        regions_.clear();
        symbols_.clear();
        references_.clear();
        applied_events_.clear();
        revision_ = core::Revision{0};
        for (const auto& event : events)
            if (const auto result = apply_locked(event); !result)
                return std::unexpected(result.error());
        return {};
    }

    /// Returns the current revision represented by this query view.
    [[nodiscard]] core::Revision current_revision() const override {
        return checkpoint();
    }

    /// Returns a copy of one exact instruction.
    [[nodiscard]] std::optional<core::Instruction> instruction_at(core::Address address) const override {
        std::scoped_lock lock(mutex_);
        const auto iterator = instructions_.find(address);
        return iterator == instructions_.end() ? std::nullopt : std::optional{iterator->second};
    }

    /// Returns a copy of one function at its entry.
    [[nodiscard]] std::optional<core::FunctionSnapshot> function_at(core::Address address) const override {
        std::scoped_lock lock(mutex_);
        const auto iterator = functions_.find(address);
        return iterator == functions_.end() ? std::nullopt : std::optional{iterator->second};
    }

    /// Returns a copy of the function whose body contains an address.
    [[nodiscard]] std::optional<core::FunctionSnapshot> function_containing(core::Address address) const override {
        std::scoped_lock lock(mutex_);
        for (const auto& [unused, function] : functions_)
            if (function.body.contains(address))
                return function;
        return std::nullopt;
    }

    /// Returns all function snapshots in entry order.
    [[nodiscard]] std::vector<core::FunctionSnapshot> functions() const override {
        std::scoped_lock lock(mutex_);
        std::vector<core::FunctionSnapshot> result;
        result.reserve(functions_.size());
        for (const auto& [unused, function] : functions_)
            result.push_back(function);
        return result;
    }

    /// Returns all instruction snapshots in address order.
    [[nodiscard]] std::vector<core::Instruction> instructions() const override {
        std::scoped_lock lock(mutex_);
        std::vector<core::Instruction> result;
        result.reserve(instructions_.size());
        for (const auto& [unused, instruction] : instructions_)
            result.push_back(instruction);
        return result;
    }

    /// Returns current data objects; data events can be added without changing this read contract.
    [[nodiscard]] std::vector<core::DataObject> data_objects() const override {
        return {};
    }

    /// Returns copies of references from one address.
    [[nodiscard]] std::vector<core::Reference> references_from(core::Address address) const override {
        std::scoped_lock lock(mutex_);
        std::vector<core::Reference> result;
        for (const auto& reference : references_)
            if (reference.source == address)
                result.push_back(reference);
        return result;
    }

    /// Returns all symbols in stable entity order.
    [[nodiscard]] std::vector<core::Symbol> symbols() const override {
        std::scoped_lock lock(mutex_);
        std::vector<core::Symbol> result;
        for (const auto& [unused, symbol] : symbols_)
            result.push_back(symbol);
        return result;
    }

    /// Returns all mapped regions in stable entity order.
    [[nodiscard]] std::vector<core::MemoryRegion> memory_regions() const override {
        std::scoped_lock lock(mutex_);
        std::vector<core::MemoryRegion> result;
        for (const auto& [unused, region] : regions_)
            result.push_back(region);
        return result;
    }

private:
    /// Applies one event while the projection mutex is held.
    [[nodiscard]] core::Result<void> apply_locked(const core::events::EventEnvelope& event) {
        if (event.project != project_)
            return std::unexpected(core::Error::make(core::DiagnosticCode::invalid_argument,
                                                     "Projection received an event for another project"));
        if (applied_events_.contains(event.event_id))
            return {};
        if (event.global_sequence <= revision_.value)
            return std::unexpected(core::Error::make(core::DiagnosticCode::projection_failure,
                                                     "Projection event sequence is not strictly increasing"));
        const auto fields = core::events::decode_fields(event.payload);
        const auto require = [&](std::string_view name) -> core::Result<std::string> {
            const auto iterator = fields.find(std::string(name));
            if (iterator == fields.end())
                return std::unexpected(core::Error::make(core::DiagnosticCode::event_corrupt,
                                                         "Projection event is missing field " + std::string(name)));
            return iterator->second;
        };
        const auto number = [&](std::string_view name, std::uint64_t fallback = 0) -> core::Result<std::uint64_t> {
            const auto iterator = fields.find(std::string(name));
            if (iterator == fields.end())
                return fallback;
            std::uint64_t value{};
            const auto [end, error] =
                std::from_chars(iterator->second.data(), iterator->second.data() + iterator->second.size(), value);
            if (error != std::errc{} || end != iterator->second.data() + iterator->second.size())
                return std::unexpected(core::Error::make(core::DiagnosticCode::event_corrupt,
                                                         "Projection event contains an invalid numeric field"));
            return value;
        };
        if (event.event_type == "MemoryStateChanged") {
            auto id = require("id");
            auto space = require("space");
            auto start = number("start");
            auto end = number("end");
            auto name = require("name");
            if (!id || !space || !start || !end || !name || *start > *end)
                return std::unexpected(
                    core::Error::make(core::DiagnosticCode::event_corrupt, "Invalid memory state event"));
            core::MemoryRegion region;
            region.id = core::EntityId{std::move(*id)};
            region.range =
                core::AddressRange{core::Address{core::AddressSpaceId{std::move(*space)}, *start},
                                   core::Address{core::AddressSpaceId{region.range.start.space.name()}, *end}};
            region.name = std::move(*name);
            region.permissions.readable = fields.contains("r") && fields.at("r") == "1";
            region.permissions.writable = fields.contains("w") && fields.at("w") == "1";
            region.permissions.executable = fields.contains("x") && fields.at("x") == "1";
            regions_[region.id] = std::move(region);
        } else if (event.event_type == "ListingStateChanged") {
            auto id = require("id");
            auto space = require("space");
            auto address = number("address");
            auto length = number("length");
            auto mnemonic = require("mnemonic");
            auto assembly = require("assembly");
            if (!id || !space || !address || !length || !mnemonic || !assembly)
                return std::unexpected(
                    core::Error::make(core::DiagnosticCode::event_corrupt, "Invalid listing state event"));
            core::Instruction instruction;
            instruction.key = core::InstructionKey{core::EntityId{std::move(*id)},
                                                   core::Address{core::AddressSpaceId{std::move(*space)}, *address}};
            instruction.length = static_cast<std::size_t>(*length);
            if (fields.contains("bytes")) {
                auto bytes = decode_hex(fields.at("bytes"));
                if (!bytes)
                    return std::unexpected(bytes.error());
                instruction.bytes = core::Bytes{std::move(*bytes)};
            }
            if (fields.contains("instruction_mask")) {
                auto mask = decode_hex(fields.at("instruction_mask"));
                if (!mask)
                    return std::unexpected(mask.error());
                instruction.instruction_mask = std::move(*mask);
            }
            if (fields.contains("operands")) {
                auto operands = decode_operands(fields.at("operands"));
                if (!operands)
                    return std::unexpected(operands.error());
                instruction.operands = std::move(*operands);
            }
            if (fields.contains("flow_kind")) {
                auto flow_kind = number("flow_kind");
                if (!flow_kind || *flow_kind > std::to_underlying(core::FlowKind::return_op))
                    return std::unexpected(
                        core::Error::make(core::DiagnosticCode::event_corrupt, "Invalid listing flow kind"));
                instruction.flow.kind = static_cast<core::FlowKind>(*flow_kind);
            }
            if (fields.contains("flow_fallthrough"))
                instruction.flow.has_fallthrough = fields.at("flow_fallthrough") == "1";
            if (fields.contains("flow_terminal"))
                instruction.flow.terminal = fields.at("flow_terminal") == "1";
            if (fields.contains("flow_target") && !fields.at("flow_target").empty()) {
                auto target = number("flow_target");
                if (!target)
                    return std::unexpected(target.error());
                instruction.flow.target = core::Address{instruction.key.address.space, *target};
            }
            instruction.mnemonic = std::move(*mnemonic);
            instruction.assembly = std::move(*assembly);
            instruction.pcode.instruction = instruction.key.address;
            instruction.provenance = event.source_service;
            instructions_[instruction.key.address] = std::move(instruction);
        } else if (event.event_type == "FunctionStateChanged") {
            auto id = require("id");
            auto space = require("space");
            auto entry = number("entry");
            auto end = number("end");
            auto name = require("name");
            if (!id || !space || !entry || !end || !name || *entry > *end)
                return std::unexpected(
                    core::Error::make(core::DiagnosticCode::event_corrupt, "Invalid function state event"));
            core::FunctionSnapshot function;
            function.key = core::FunctionKey{core::EntityId{std::move(*id)},
                                             core::Address{core::AddressSpaceId{std::move(*space)}, *entry}};
            function.name = std::move(*name);
            function.body.add(core::AddressRange{function.key.entry, core::Address{function.key.entry.space, *end}});
            function.analysis_status = fields.contains("status") ? fields.at("status") : "materialized";
            functions_[function.key.entry] = std::move(function);
        } else if (event.event_type == "SymbolStateChanged") {
            auto id = require("id");
            auto name = require("name");
            if (!id || !name)
                return std::unexpected(
                    core::Error::make(core::DiagnosticCode::event_corrupt, "Invalid symbol state event"));
            core::Symbol symbol;
            symbol.id = core::EntityId{std::move(*id)};
            symbol.name = std::move(*name);
            symbol.namespace_name = fields.contains("namespace") ? fields.at("namespace") : "";
            if (const auto address = fields.find("address"); address != fields.end() && !address->second.empty())
                symbol.address = core::Address{core::AddressSpaceId{"ram"}, std::stoull(address->second)};
            if (const auto priority = fields.find("priority"); priority != fields.end())
                symbol.source_priority = std::stoi(priority->second);
            symbols_[symbol.id] = std::move(symbol);
        }
        applied_events_.insert(event.event_id);
        revision_ = core::Revision{event.global_sequence};
        return {};
    }

    core::ProjectId project_;
    mutable std::mutex mutex_;
    core::Revision revision_;
    std::set<core::EventId> applied_events_;
    std::map<core::Address, core::Instruction> instructions_;
    std::map<core::Address, core::FunctionSnapshot> functions_;
    std::map<core::EntityId, core::MemoryRegion> regions_;
    std::map<core::EntityId, core::Symbol> symbols_;
    std::vector<core::Reference> references_;
};

} // namespace recode::runtime::projections
