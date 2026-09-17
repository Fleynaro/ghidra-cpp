import decompiler;
import recode.decompiler;
import hello;
import analyzer;
import sleigh_runtime;
import std;

namespace {

/// Supplies one deterministic COPY followed by RETURN instruction to the frontend.
class MinimalSmokeProvider final : public recode::decompiler::PcodeProvider {
public:
    /// Constructs the fixed one-byte provider used by the application smoke test.
    MinimalSmokeProvider() = default;

    /// Returns the fixed instruction at address zero and rejects all other addresses.
    [[nodiscard]] std::expected<recode::decompiler::Instruction, recode::decompiler::ProviderError>
    decode(std::uint64_t address) const override {
        if (address != 0) {
            return std::unexpected(
                recode::decompiler::ProviderError{"MinimalSmokeProvider has no instruction at this address"});
        }

        recode::decompiler::Instruction instruction;
        instruction.address = 0;
        instruction.length = 1;
        instruction.mnemonic = "mov";
        instruction.assembly = "rax, 7";
        instruction.pcode.push_back(recode::decompiler::PcodeOperation{
            std::to_underlying(sleigh_runtime::PcodeOpcode::copy),
            recode::decompiler::Storage{"register", 0, 8},
            {recode::decompiler::Storage{"const", 7, 8}},
        });
        instruction.pcode.push_back(recode::decompiler::PcodeOperation{
            std::to_underlying(sleigh_runtime::PcodeOpcode::return_op),
            std::nullopt,
            {recode::decompiler::Storage{"const", 0, 8}},
        });
        return instruction;
    }
};

/// Builds the minimal register and address-space model required by the smoke case.
[[nodiscard]] recode::decompiler::ArchitectureDescription smoke_architecture() {
    recode::decompiler::ArchitectureDescription description;
    description.name = "recode-smoke-x86-64";
    description.calling_convention = "__cdecl";
    description.spaces = {
        recode::decompiler::SpaceDescription{"ram", 8, 1, false, 2, 0, true},
        recode::decompiler::SpaceDescription{"register", 8, 1, false, 3, 0, true},
    };
    description.registers = {
        recode::decompiler::RegisterDescription{"RAX", {"register", 0, 8}},
        recode::decompiler::RegisterDescription{"RCX", {"register", 8, 8}},
        recode::decompiler::RegisterDescription{"RDX", {"register", 0x10, 8}},
        recode::decompiler::RegisterDescription{"RSP", {"register", 0x20, 8}},
        recode::decompiler::RegisterDescription{"R8", {"register", 0x80, 8}},
        recode::decompiler::RegisterDescription{"R9", {"register", 0x88, 8}},
    };
    return description;
}

/// Runs the deterministic provider frontend smoke case and verifies generated C output.
[[nodiscard]] int run_decompiler_smoke() {
    auto provider = std::make_shared<MinimalSmokeProvider>();
    auto memory = std::make_shared<recode::decompiler::SparseMemory>(0, std::vector<std::uint8_t>{0x90});
    recode::decompiler::Decompiler decompiler(smoke_architecture(), provider, memory);
    const recode::decompiler::DecompilationResult result =
        decompiler.decompile(recode::decompiler::FunctionDescription{"app_smoke", 0, 1});
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
int main(int argc, char** argv) {
    if (argc == 3) {
        // Production path: PE Loader owns image state, Sleigh owns decoding,
        // and AutoAnalysisManager owns the event-driven analyzer pipeline.
        auto image = pe::PeLoader::load_file(argv[1]);
        if (!image) {
            std::cerr << "PE analysis failed: " << image.error().message << '\n';
            return 1;
        }
        try {
            recode::analyzer::AnalysisContext context(std::move(*image), argv[2]);
            recode::analyzer::AutoAnalysisManager manager(context);
            manager.register_builtin_analyzers();
            const auto result = manager.analyze();
            if (!result.completed) {
                for (const auto& error : result.errors)
                    std::cerr << error << '\n';
                return result.cancelled ? 2 : 1;
            }
            std::cout << "instructions=" << context.instructions().size() << " functions=" << context.functions().size()
                      << " references=" << context.references().size() << '\n';
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "PE analysis failed: " << error.what() << '\n';
            return 1;
        }
    }
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
