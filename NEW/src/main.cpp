import decompiler;
import ghidra.decompiler;
import hello;
import sleigh_runtime;
import std;

namespace {

/// Supplies one deterministic COPY followed by RETURN instruction to the frontend.
class MinimalSmokeProvider final : public newghidra::decompiler::PcodeProvider {
public:
    /// Constructs the fixed one-byte provider used by the application smoke test.
    MinimalSmokeProvider() = default;

    /// Returns the fixed instruction at address zero and rejects all other addresses.
    [[nodiscard]] std::expected<newghidra::decompiler::Instruction, newghidra::decompiler::ProviderError>
    decode(std::uint64_t address) const override {
        if (address != 0) {
            return std::unexpected(
                newghidra::decompiler::ProviderError{"MinimalSmokeProvider has no instruction at this address"});
        }

        newghidra::decompiler::Instruction instruction;
        instruction.address = 0;
        instruction.length = 1;
        instruction.mnemonic = "mov";
        instruction.assembly = "rax, 7";
        instruction.pcode.push_back(newghidra::decompiler::PcodeOperation{
            std::to_underlying(sleigh_runtime::PcodeOpcode::copy),
            newghidra::decompiler::Storage{"register", 0, 8},
            {newghidra::decompiler::Storage{"const", 7, 8}},
        });
        instruction.pcode.push_back(newghidra::decompiler::PcodeOperation{
            std::to_underlying(sleigh_runtime::PcodeOpcode::return_op),
            std::nullopt,
            {newghidra::decompiler::Storage{"const", 0, 8}},
        });
        return instruction;
    }
};

/// Builds the minimal register and address-space model required by the smoke case.
[[nodiscard]] newghidra::decompiler::ArchitectureDescription smoke_architecture() {
    newghidra::decompiler::ArchitectureDescription description;
    description.name = "new-ghidra-smoke-x86-64";
    description.calling_convention = "__cdecl";
    description.spaces = {
        newghidra::decompiler::SpaceDescription{"ram", 8, 1, false, 2, 0, true},
        newghidra::decompiler::SpaceDescription{"register", 8, 1, false, 3, 0, true},
    };
    description.registers = {
        newghidra::decompiler::RegisterDescription{"RAX", {"register", 0, 8}},
        newghidra::decompiler::RegisterDescription{"RCX", {"register", 8, 8}},
        newghidra::decompiler::RegisterDescription{"RDX", {"register", 0x10, 8}},
        newghidra::decompiler::RegisterDescription{"RSP", {"register", 0x20, 8}},
        newghidra::decompiler::RegisterDescription{"R8", {"register", 0x80, 8}},
        newghidra::decompiler::RegisterDescription{"R9", {"register", 0x88, 8}},
    };
    return description;
}

/// Runs the deterministic provider frontend smoke case and verifies generated C output.
[[nodiscard]] int run_decompiler_smoke() {
    auto provider = std::make_shared<MinimalSmokeProvider>();
    auto memory = std::make_shared<newghidra::decompiler::SparseMemory>(0, std::vector<std::uint8_t>{0x90});
    newghidra::decompiler::Decompiler decompiler(smoke_architecture(), provider, memory);
    const newghidra::decompiler::DecompilationResult result =
        decompiler.decompile(newghidra::decompiler::FunctionDescription{"app_smoke", 0, 1});
    const std::string expected_c = "\nundefined8 __cdecl app_smoke(void)\n\n{\n  return 7;\n}\n";
    if (result.c_source != expected_c) {
        std::cerr << "Decompiler smoke output did not match the deterministic expectation.\n";
        return 1;
    }
    std::cout << result.c_source;
    return 0;
}

} // namespace

/// Runs the existing hello demonstration and the standalone decompiler smoke case.
int main() {
    if (hello::run_demo() != 0) {
        return 1;
    }
    try {
        return run_decompiler_smoke();
    } catch (const ghidra::LowlevelError& error) {
        std::cerr << "Decompiler smoke failed: " << error.explain << '\n';
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "Decompiler smoke failed: " << error.what() << '\n';
        return 1;
    }
}
