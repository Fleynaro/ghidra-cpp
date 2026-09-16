export module ghidra.core.contracts.decompiler;

import std;
import ghidra.core.decompilation;
import ghidra.core.diagnostics;
import ghidra.core.function;
import ghidra.core.contracts.memory_provider;
import ghidra.core.contracts.operation;
import ghidra.core.contracts.pcode_decoder;
import ghidra.core.contracts.project_query;

export namespace ghidra::core::contracts {

/// Bundles immutable providers for one native decompiler session.
struct ProviderContext {
    std::shared_ptr<const IPCodeDecoder> pcode;
    std::shared_ptr<const IMemoryProvider> memory;
    std::shared_ptr<const IProjectQuery> project;
};

/// Requests decompilation of one revision-stamped function snapshot.
struct DecompileRequest {
    FunctionSnapshot function;
    Revision read_revision;
    ProviderContext providers;
    bool include_text{true};
    bool include_control_flow{true};
};

/// Runs the native decompiler against immutable providers.
class IDecompiler {
public:
    /// Releases a decompiler service through its contract.
    virtual ~IDecompiler() = default;

    /// Queues an expensive decompilation task.
    [[nodiscard]] virtual Task<Result<Decompilation>> decompile(DecompileRequest request, OperationContext context) = 0;

    /// Runs one explicitly synchronous test/CLI decompilation.
    [[nodiscard]] virtual Result<Decompilation> decompile_now(const DecompileRequest& request) = 0;
};

} // namespace ghidra::core::contracts
