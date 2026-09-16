export module ghidra.core.contracts.pcode_decoder;

import std;
import ghidra.core.address;
import ghidra.core.bytes;
import ghidra.core.instruction;
import ghidra.core.processor_context;
import ghidra.core.contracts.operation;

export namespace ghidra::core::contracts {

/// Requests one bounded instruction decode.
struct DecodeRequest {
    Address address;
    Bytes bytes;
    ProcessorContext context;
};

/// Requests deterministic decoding of bounded address/byte pairs.
struct DecodeBatchRequest {
    std::vector<DecodeRequest> requests;
};

/// Returns address-ordered instructions from a batch operation.
struct DecodeBatchResult {
    std::vector<Instruction> instructions;
};

/// Decodes machine bytes into canonical core values.
class IPCodeDecoder {
public:
    /// Releases a decoder through its contract.
    virtual ~IPCodeDecoder() = default;

    /// Decodes one instruction synchronously.
    [[nodiscard]] virtual Result<Instruction> decode(const DecodeRequest& request) const = 0;

    /// Queues a bounded batch operation through the owning runtime.
    [[nodiscard]] virtual Task<Result<DecodeBatchResult>> decode_batch(const DecodeBatchRequest& request,
                                                                       OperationContext context) const = 0;
};

} // namespace ghidra::core::contracts
