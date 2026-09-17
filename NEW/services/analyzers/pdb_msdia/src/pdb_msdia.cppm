module;

#ifdef _WIN32
#include <dia2.h>
#include <oleauto.h>
#include <windows.h>
#endif

export module analyzer_pdb_msdia;

import analyzer;
import std;

// Ported behavior is traced to
// Ghidra/Features/PDB/src/main/java/ghidra/app/plugin/core/analysis/
// PdbMsdiaAnalyzer.java, PdbAnalyzer.java, and PdbAnalyzerCommon.java.

export namespace recode::pdb::msdia {

/// Reports provider, COM, file, or DIA enumeration failures.
struct MsdiaError {
    std::string message;
};

/// Represents one symbol obtained through the DIA SDK rather than a name-only shim.
struct MsdiaSymbol {
    std::string name;
    std::string tag;
    std::uint64_t virtual_address{};
    std::uint32_t length{};
    bool function{};
};

/// Owns one real DIA data source/session and its enumerated symbols.
class MsdiaSession final {
public:
    /// Returns whether this build has a Windows DIA provider boundary.
    [[nodiscard]] static constexpr bool platform_supported() noexcept {
#ifdef _WIN32
        return true;
#else
        return false;
#endif
    }

    /// Opens a PDB through DIA and enumerates function/data symbol records.
    [[nodiscard]] static std::expected<MsdiaSession, MsdiaError> open(const std::filesystem::path& path);

    /// Returns the symbols retained from the DIA enumeration.
    [[nodiscard]] const std::vector<MsdiaSymbol>& symbols() const noexcept {
        return symbols_;
    }

    /// Returns the original PDB path associated with the session.
    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return path_;
    }

    /// Releases COM/DIA state before the session leaves scope.
    ~MsdiaSession();

    /// Prevents copying a COM session.
    MsdiaSession(const MsdiaSession&) = delete;

    /// Prevents copying a COM session.
    MsdiaSession& operator=(const MsdiaSession&) = delete;

    /// Transfers COM/DIA ownership to a new session.
    MsdiaSession(MsdiaSession&& other) noexcept;

    /// Transfers COM/DIA ownership to an existing session.
    MsdiaSession& operator=(MsdiaSession&& other) noexcept;

private:
    /// Stores the portable session result before platform-specific handles are attached.
    explicit MsdiaSession(std::filesystem::path path) : path_(std::move(path)) {}

    std::filesystem::path path_;
    std::vector<MsdiaSymbol> symbols_;
#ifdef _WIN32
    IDiaDataSource* data_source_{};
    IDiaSession* session_{};
    bool com_initialized_{};
#endif
};

} // namespace recode::pdb::msdia

export namespace recode::analyzer {

/// Applies symbols from the Windows DIA provider boundary.
class PdbMsdiaAnalyzer final : public Analyzer {
public:
    /// Returns the DIA analyzer scheduling contract.
    [[nodiscard]] AnalyzerDescriptor descriptor() const override;

    /// Opens DIA on Windows or reports its explicit platform error elsewhere.
    void analyze(AnalysisContext&, std::span<const AnalysisEvent>, CancellationToken&) override;
};

} // namespace recode::analyzer

namespace recode::pdb::msdia {
namespace {

/// Converts a failed HRESULT to a stable diagnostic without hiding its code.
[[nodiscard]] MsdiaError hresult_error(std::string_view operation, long result) {
    std::ostringstream message;
    message << "PDB MSDIA: " << operation << " failed with HRESULT 0x" << std::hex
            << static_cast<unsigned long>(result);
    return MsdiaError{message.str()};
}

#ifdef _WIN32

/// Releases one DIA COM interface pointer.
template <typename Interface> void release(Interface*& pointer) noexcept {
    if (pointer != nullptr) {
        pointer->Release();
        pointer = nullptr;
    }
}

/// Converts a DIA BSTR into UTF-8-compatible narrow text for the native model.
[[nodiscard]] std::string narrow(BSTR value) {
    if (value == nullptr)
        return {};
    const int required =
        WideCharToMultiByte(CP_UTF8, 0, value, static_cast<int>(SysStringLen(value)), nullptr, 0, nullptr, nullptr);
    if (required <= 0)
        return {};
    std::string result(static_cast<std::size_t>(required), '\0');
    static_cast<void>(WideCharToMultiByte(CP_UTF8, 0, value, static_cast<int>(SysStringLen(value)), result.data(),
                                          required, nullptr, nullptr));
    return result;
}

/// Enumerates one DIA symbol collection into portable records.
[[nodiscard]] std::expected<void, MsdiaError> enumerate(IDiaEnumSymbols* enumeration, bool function,
                                                        std::vector<MsdiaSymbol>& output) {
    if (enumeration == nullptr)
        return {};
    while (true) {
        IDiaSymbol* symbol = nullptr;
        ULONG fetched = 0;
        const HRESULT result = enumeration->Next(1, &symbol, &fetched);
        if (result == S_FALSE || fetched == 0U)
            break;
        if (FAILED(result))
            return std::unexpected(hresult_error("IDiaEnumSymbols::Next", result));
        if (symbol == nullptr)
            continue;
        BSTR name = nullptr;
        ULONGLONG address = 0;
        ULONGLONG length = 0;
        const HRESULT name_result = symbol->get_name(&name);
        const HRESULT address_result = symbol->get_virtualAddress(&address);
        const HRESULT length_result = symbol->get_length(&length);
        if (SUCCEEDED(name_result) && SUCCEEDED(address_result) && SUCCEEDED(length_result) && name != nullptr &&
            address != 0U) {
            output.push_back({narrow(name), function ? "Function" : "Data", address,
                              static_cast<std::uint32_t>(std::min<ULONGLONG>(length, 0xffffffffULL)), function});
        }
        if (name != nullptr)
            SysFreeString(name);
        symbol->Release();
    }
    return {};
}

#endif

} // namespace

/// Opens the actual Microsoft DIA COM provider and enumerates supported symbols.
std::expected<MsdiaSession, MsdiaError> MsdiaSession::open(const std::filesystem::path& path) {
#ifndef _WIN32
    static_cast<void>(path);
    return std::unexpected(
        MsdiaError{"PDB MSDIA requires Windows and the Microsoft DIA SDK; this build is non-Windows"});
#else
    if (path.empty())
        return std::unexpected(MsdiaError{"PDB MSDIA: no PDB path was supplied"});
    if (!std::filesystem::exists(path))
        return std::unexpected(MsdiaError{"PDB MSDIA: PDB path does not exist: " + path.string()});
    MsdiaSession result(path);
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE)
        return std::unexpected(hresult_error("CoInitializeEx", initialized));
    result.com_initialized_ = SUCCEEDED(initialized);
    HRESULT created = CoCreateInstance(__uuidof(DiaSource), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IDiaDataSource),
                                       reinterpret_cast<void**>(&result.data_source_));
    if (FAILED(created)) {
        if (result.com_initialized_)
            CoUninitialize();
        return std::unexpected(hresult_error("CoCreateInstance(DiaSource)", created));
    }
    created = result.data_source_->loadDataFromPdb(path.c_str());
    if (FAILED(created)) {
        result.data_source_->Release();
        result.data_source_ = nullptr;
        if (result.com_initialized_)
            CoUninitialize();
        return std::unexpected(hresult_error("IDiaDataSource::loadDataFromPdb", created));
    }
    created = result.data_source_->openSession(&result.session_);
    if (FAILED(created)) {
        result.data_source_->Release();
        result.data_source_ = nullptr;
        if (result.com_initialized_)
            CoUninitialize();
        return std::unexpected(hresult_error("IDiaDataSource::openSession", created));
    }
    IDiaSymbol* global = nullptr;
    created = result.session_->get_globalScope(&global);
    if (FAILED(created) || global == nullptr) {
        if (global != nullptr)
            global->Release();
        return std::unexpected(hresult_error("IDiaSession::get_globalScope", created));
    }
    IDiaEnumSymbols* functions = nullptr;
    created = global->findChildren(SymTagFunction, nullptr, nsNone, &functions);
    if (FAILED(created)) {
        global->Release();
        return std::unexpected(hresult_error("IDiaSymbol::findChildren(function)", created));
    }
    auto function_result = enumerate(functions, true, result.symbols_);
    if (functions != nullptr)
        functions->Release();
    if (!function_result) {
        global->Release();
        return std::unexpected(function_result.error());
    }
    IDiaEnumSymbols* data = nullptr;
    created = global->findChildren(SymTagData, nullptr, nsNone, &data);
    if (SUCCEEDED(created)) {
        auto data_result = enumerate(data, false, result.symbols_);
        if (data != nullptr)
            data->Release();
        if (!data_result) {
            global->Release();
            return std::unexpected(data_result.error());
        }
    }
    global->Release();
    std::sort(result.symbols_.begin(), result.symbols_.end(), [](const MsdiaSymbol& left, const MsdiaSymbol& right) {
        return std::tie(left.virtual_address, left.name) < std::tie(right.virtual_address, right.name);
    });
    return result;
#endif
}

/// Releases native DIA and COM ownership.
MsdiaSession::~MsdiaSession() {
#ifdef _WIN32
    release(session_);
    release(data_source_);
    if (com_initialized_)
        CoUninitialize();
#endif
}

/// Transfers native DIA and COM ownership.
MsdiaSession::MsdiaSession(MsdiaSession&& other) noexcept
    : path_(std::move(other.path_)), symbols_(std::move(other.symbols_))
#ifdef _WIN32
      ,
      data_source_(std::exchange(other.data_source_, nullptr)), session_(std::exchange(other.session_, nullptr)),
      com_initialized_(std::exchange(other.com_initialized_, false))
#endif
{
}

/// Transfers native DIA and COM ownership to an existing object.
MsdiaSession& MsdiaSession::operator=(MsdiaSession&& other) noexcept {
    if (this == &other)
        return *this;
#ifdef _WIN32
    release(session_);
    release(data_source_);
    if (com_initialized_)
        CoUninitialize();
#endif
    path_ = std::move(other.path_);
    symbols_ = std::move(other.symbols_);
#ifdef _WIN32
    data_source_ = std::exchange(other.data_source_, nullptr);
    session_ = std::exchange(other.session_, nullptr);
    com_initialized_ = std::exchange(other.com_initialized_, false);
#endif
    return *this;
}

} // namespace recode::pdb::msdia

namespace recode::analyzer {

/// Returns the Windows-only DIA analyzer scheduling contract.
AnalyzerDescriptor PdbMsdiaAnalyzer::descriptor() const {
    return {"PDB MSDIA", 899, {EventKind::memory_added}, {}};
}

/// Applies DIA function and data symbols, with a deterministic non-Windows error.
void PdbMsdiaAnalyzer::analyze(AnalysisContext& context, std::span<const AnalysisEvent>,
                               CancellationToken& cancellation) {
    if (!context.options().pdb_msdia || context.options().pdb_path.empty() || cancellation.is_cancelled())
        return;
    const auto session = recode::pdb::msdia::MsdiaSession::open(context.options().pdb_path);
    if (!session)
        throw std::runtime_error(session.error().message);
    for (const auto& symbol : session->symbols()) {
        if (cancellation.is_cancelled())
            return;
        if (!context.image().find_memory_region(symbol.virtual_address))
            continue;
        const std::string kind = symbol.function ? "function" : "data";
        static_cast<void>(context.add_pdb_symbol(
            PdbSymbolRecord{symbol.virtual_address, symbol.name, {}, kind, symbol.length, symbol.function, false}));
        static_cast<void>(
            context.add_symbol(SymbolRecord{symbol.virtual_address, {}, symbol.name, {}, kind, false, true}));
        if (symbol.function) {
            if (!context.instructions().contains(symbol.virtual_address))
                static_cast<void>(context.disassemble_flow(symbol.virtual_address));
            if (!context.functions().contains(symbol.virtual_address))
                static_cast<void>(context.create_function(symbol.virtual_address, symbol.name));
            static_cast<void>(context.set_function_name(symbol.virtual_address, symbol.name));
        } else if (symbol.length != 0U) {
            static_cast<void>(context.add_data(DataObject{symbol.virtual_address, symbol.length, "pdb_data"}));
        }
    }
    if (context.options().create_analysis_bookmarks)
        static_cast<void>(context.add_bookmark(Bookmark{context.image().optional_header().image_base, "PDB MSDIA",
                                                        "DIA provider loaded: " + session->path().string()}));
}

} // namespace recode::analyzer
