export module recode.core.contracts.pcode_decoder;

import std;
import recode.core.address;
import recode.core.bytes;
import recode.core.decoded_instruction;
import recode.core.processor_context;
import recode.core.contracts.operation;

export namespace recode::core::contracts {

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
    std::vector<DecodedInstruction> instructions;
};

/// Decodes machine bytes into canonical core values.
class IPCodeDecoder {
public:
    /// Releases a decoder through its contract.
    virtual ~IPCodeDecoder() = default;

    /// Decodes one instruction synchronously.
    [[nodiscard]] virtual Result<DecodedInstruction> decode(const DecodeRequest& request) const = 0;

    /// Queues a bounded batch operation through the owning runtime.
    [[nodiscard]] virtual Task<Result<DecodeBatchResult>> decode_batch(const DecodeBatchRequest& request,
                                                                       OperationContext context) const = 0;
};

} // namespace recode::core::contracts
