// MSVC x64 end-to-end fixture for the complete native analyzer registration.
//
// This executable intentionally combines the behavioral inputs used by the
// focused analyzer fixtures. The corresponding original Ghidra contracts are:
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/AutoAnalysisManager.java
//   and AnalysisOptions.java for scheduling and option interactions.
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/function/
//   FunctionAnalyzer.java, StackVariableAnalyzer.java, and
//   ExternalEntryFunctionAnalyzer.java for discovery, bodies, and frames.
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/disassembler/
//   EntryPointAnalyzer.java, AddressTableAnalyzer.java, and CallFixupAnalyzer.java.
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/
//   OperandReferenceAnalyzer.java, ConstantPropagationAnalyzer.java,
//   NonReturnFunctionAnalyzer.java, and MicrosoftDemanglerAnalyzer.java.
// * Ghidra/Features/Decompiler/src/main/java/ghidra/app/plugin/core/analysis/
//   DecompilerParameterIdAnalyzer.java and DecompilerSwitchAnalyzer.java.
// * Ghidra/Features/DecompilerDependent/src/main/java/ghidra/app/plugin/core/string/
//   variadic/FormatStringAnalyzer.java and
//   Ghidra/Features/MicrosoftCodeAnalyzer/src/main/java/ghidra/app/plugin/
//   prototype/MicrosoftCodeAnalyzerPlugin/WindowsResourceReferenceAnalyzer.java.
// * Ghidra/Features/Base/src/main/java/ghidra/app/plugin/core/analysis/
//   EmbeddedMediaAnalyzer.java, StringsAnalyzer.java, CondenseFillerBytesAnalyzer.java,
//   and AggressiveInstructionFinderAnalyzer.java.
// * Ghidra/Features/FunctionID, Ghidra/Features/PDB, and
//   Ghidra/Features/BytePatterns for their respective registered phases.
//
// No assembly is manufactured here. All code, data, imports, relocations,
// resource records, and compiler padding are produced by the normal MSVC and
// Windows linker pipeline described by build.bat.

#include "resource.h"

#pragma section(".rdata$media", read)
#pragma section(".rdata$engine", read)

extern "C" __declspec(dllimport) int __stdcall LoadStringW(void* instance, unsigned int identifier, wchar_t* buffer,
                                                           int buffer_length);
extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(const char* message);
extern "C" __declspec(dllimport) unsigned long __stdcall GetTickCount(void);
extern "C" __declspec(dllimport) __declspec(noreturn) void __stdcall ExitProcess(unsigned int code);
// MSVC emits this marker when floating-point code is linked without the CRT.
extern "C" int _fltused = 0;

extern "C" __declspec(dllexport) volatile unsigned int engine_state = 0U;
extern "C" __declspec(dllexport) volatile int engine_selector = 3;
extern "C" __declspec(dllexport) volatile unsigned long long engine_global_ticks = 0ULL;
static volatile unsigned int engine_static_cache = 0U;

// These initialized values provide ordinary global, array, structure, pointer,
// and cross-section data references for Reference and Data Reference.
struct EngineConfig {
    unsigned int world_id;
    const char* display_name;
    const int* difficulty_levels;
    unsigned int difficulty_count;
};

namespace demangle_fixture {

/// Provides exported C++ member methods for Microsoft demangling in the full image.
class Calculator {
public:
    /// Adds two values while retaining a decorated instance-method symbol.
    __declspec(dllexport) __declspec(noinline) int add(int left, int right);

    /// Scales one value while retaining a decorated static-method symbol.
    __declspec(dllexport) __declspec(noinline) static int scale(int value);
};

/// Implements the decorated instance method and updates shared engine state.
int Calculator::add(int left, int right) {
    engine_state = static_cast<unsigned int>(left + right);
    return static_cast<int>(engine_state);
}

/// Implements the decorated static method and updates shared engine state.
int Calculator::scale(int value) {
    engine_state = static_cast<unsigned int>(value * 3);
    return static_cast<int>(engine_state);
}

} // namespace demangle_fixture

namespace overload_fixture {

/// Adds integral operands through the first genuine overload.
__declspec(dllexport) __declspec(noinline) int combine(int left, int right) {
    return left + right + 7;
}

/// Adds floating operands through the second genuine overload.
__declspec(dllexport) __declspec(noinline) float combine(float left, float right) {
    return left * 2.0f + right * 0.25f;
}

} // namespace overload_fixture

extern "C" __declspec(dllexport) const char engine_banner[] = "GTA-like native analysis integration fixture";
extern "C" __declspec(dllexport) const char engine_format[] = "tick=%d entity=%s score=%u";
extern "C" __declspec(dllexport) const char engine_short[] = "tiny";
extern "C" __declspec(dllexport)
const char engine_unterminated[] = {'u', 'n', 't', 'e', 'r', 'm', 'i', 'n', 'a', 't', 'e', 'd'};
extern "C" __declspec(dllexport) const unsigned char engine_non_ascii[] = {0xc3, 0xa9, 0xc3, 0xa0, 0xc3, 0xb1, 0x00};
extern "C" __declspec(dllexport) const int engine_difficulty_levels[] = {1, 3, 7, 12, 24};
extern "C" __declspec(dllexport) EngineConfig engine_config = {0x475441U, engine_banner, engine_difficulty_levels, 5U};

// These are valid media containers, not marker-only byte sequences. Embedded
// Media must validate their structures before creating data objects.
extern "C" __declspec(dllexport) __declspec(allocate(".rdata$media"))
const unsigned char engine_gif[] = {'G', 'I', 'F', '8', '9', 'a', 1, 0, 1, 0, 0x80, 0, 0, 0,    0, 0, 0,  0,
                                    0,   ',', 0,   0,   0,   0,   1, 0, 1, 0, 0,    2, 2, 0x44, 1, 0, ';'};
extern "C" __declspec(dllexport) __declspec(allocate(".rdata$media")) const unsigned char engine_png[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0,    0,    0,    0x0d, 'I',  'H',  'D',  'R',  0,
    0,    0,    1,    0,    0,    0,    1,    8,    4,    0,    0,    0,    0xb5, 0x1c, 0x0c, 2,    0,
    0,    0,    0x0b, 'I',  'D',  'A',  'T',  0x78, 0xda, 0x63, 0x64, 0xf8, 0x0f, 0,    1,    5,    1,
    1,    0x27, 0x18, 0xe3, 0x66, 0,    0,    0,    0,    'I',  'E',  'N',  'D',  0xae, 0x42, 0x60, 0x82};
extern "C" __declspec(dllexport) __declspec(allocate(".rdata$media")) const unsigned char engine_wave[] = {
    'R', 'I', 'F',  'F',  0x24, 0, 0,    0,    'W', 'A', 'V', 'E', 'f',  'm', 't', ' ', 0x10, 0,   0, 0, 1, 0,
    1,   0,   0x44, 0xac, 0,    0, 0x88, 0x58, 1,   0,   2,   0,   0x10, 0,   'd', 'a', 't',  'a', 0, 0, 0, 0};
extern "C" __declspec(dllexport) __declspec(allocate(".rdata$media")) const unsigned char engine_midi[] = {
    'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 0, 0x60, 'M', 'T', 'r', 'k', 0, 0, 0, 4, 0, 0xff, 0x2f, 0};
extern "C" __declspec(dllexport) __declspec(allocate(".rdata$media")) const unsigned char engine_au[] = {
    '.', 's', 'n', 'd', 0, 0, 0, 0x18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0};

namespace engine {

/// Represents an object in the common engine entity hierarchy.
struct Entity {
    /// Constructs an entity with stable identity and display metadata.
    Entity(int entity_id, const char* entity_name) : id(entity_id), name(entity_name) {}

    /// Provides a non-virtual destructor used by stack-owned hierarchy objects.
    ~Entity() {}

    /// Updates the base entity and returns its deterministic score contribution.
    virtual int update(int tick) {
        engine_state += static_cast<unsigned int>(id + tick);
        return id + tick;
    }

    /// Returns the base runtime kind string through a virtual dispatch path.
    virtual const char* kind() const {
        return "Entity";
    }

    int id;
    const char* name;
};

/// Represents a vehicle specialization with overridden and non-virtual behavior.
struct Vehicle : Entity {
    /// Constructs a vehicle and initializes its fuel state.
    Vehicle(int entity_id, const char* entity_name, int initial_fuel)
        : Entity(entity_id, entity_name), fuel(initial_fuel) {}

    /// Destroys a stack-owned vehicle without requiring a runtime library.
    ~Vehicle() {
        fuel = 0;
    }

    /// Overrides Entity::update with vehicle-specific arithmetic and state writes.
    int update(int tick) override {
        fuel -= tick & 3;
        engine_state += static_cast<unsigned int>(fuel);
        return Entity::update(tick) + fuel;
    }

    /// Returns the derived runtime kind through the vtable.
    const char* kind() const override {
        return "Vehicle";
    }

    /// Refuels the vehicle through a non-virtual member call.
    void refuel(int amount) {
        fuel += amount;
    }

    int fuel;
};

/// Represents a pedestrian specialization with a separate callback-like action.
struct Ped : Entity {
    /// Constructs a pedestrian with a deterministic health value.
    Ped(int entity_id, const char* entity_name, int initial_health)
        : Entity(entity_id, entity_name), health(initial_health) {}

    /// Destroys a stack-owned pedestrian.
    ~Ped() {
        health = 0;
    }

    /// Overrides Entity::update with health and identity arithmetic.
    int update(int tick) override {
        health += (tick ^ id) & 7;
        return Entity::update(tick) + health;
    }

    /// Returns the derived runtime kind through the vtable.
    const char* kind() const override {
        return "Ped";
    }

    /// Applies a deterministic damage operation to the pedestrian.
    void damage(int amount) {
        health -= amount;
    }

    int health;
};

/// Represents a player specialization below Ped, exercising a second override level.
struct Player : Ped {
    /// Constructs a player with a selected weapon handle.
    Player(int entity_id, const char* entity_name, int initial_health, unsigned int weapon_handle)
        : Ped(entity_id, entity_name, initial_health), selected_weapon(weapon_handle) {}

    /// Destroys a stack-owned player and clears the selected handle.
    ~Player() {
        selected_weapon = 0U;
    }

    /// Overrides Ped::update and records the player handle in global state.
    int update(int tick) override {
        engine_global_ticks ^= static_cast<unsigned long long>(selected_weapon + tick);
        return Ped::update(tick) + static_cast<int>(selected_weapon & 0xffU);
    }

    /// Returns the player-specific runtime kind through the vtable.
    const char* kind() const override {
        return "Player";
    }

    /// Changes the selected weapon without virtual dispatch.
    void select_weapon(unsigned int handle) {
        selected_weapon = handle;
    }

    unsigned int selected_weapon;
};

/// Represents a second, resource-oriented hierarchy unrelated to Entity.
struct Asset {
    /// Constructs an asset with a resource identifier.
    explicit Asset(unsigned int resource_id) : id(resource_id) {}

    /// Provides a non-virtual destructor for stack-owned assets.
    ~Asset() {}

    /// Returns the base asset size contribution.
    virtual unsigned int size() const {
        return id & 0xffU;
    }

    unsigned int id;
};

/// Represents a texture resource with an overridden size calculation.
struct TextureAsset : Asset {
    /// Constructs a texture resource with dimensions.
    TextureAsset(unsigned int resource_id, unsigned int width, unsigned int height)
        : Asset(resource_id), width(width), height(height) {}

    /// Returns the texture byte estimate through virtual dispatch.
    unsigned int size() const override {
        return width * height * 4U + Asset::size();
    }

    unsigned int width;
    unsigned int height;
};

/// Represents a script resource with a non-virtual checksum operation.
struct ScriptAsset : Asset {
    /// Constructs a script resource with a byte count.
    ScriptAsset(unsigned int resource_id, unsigned int byte_count) : Asset(resource_id), bytes(byte_count) {}

    /// Returns the script byte count through virtual dispatch.
    unsigned int size() const override {
        return bytes + Asset::size();
    }

    /// Computes a stable script resource checksum.
    unsigned int checksum(unsigned int seed) const {
        return (seed ^ id) + bytes * 17U;
    }

    unsigned int bytes;
};

} // namespace engine

using EngineCallback = unsigned int (*)(unsigned int);

extern "C" __declspec(dllexport) __declspec(noinline) unsigned int callback_add(unsigned int value) {
    return value + 0x11U;
}

extern "C" __declspec(dllexport) __declspec(noinline) unsigned int callback_rotate(unsigned int value) {
    return (value << 3U) | (value >> 29U);
}

extern "C" __declspec(dllexport) __declspec(noinline) unsigned int callback_mask(unsigned int value) {
    return (value ^ 0xa5a5a5a5U) & 0x00ffffffU;
}

// The table is deliberately in undefined initialized read-only storage. Create
// Address Tables and Data Reference must agree on its pointer run and boundary.
extern "C" __declspec(dllexport) __declspec(allocate(".rdata$engine")) const EngineCallback engine_callbacks[] = {
    callback_add,
    callback_rotate,
    callback_mask,
    callback_add,
};
extern "C" __declspec(dllexport) __declspec(allocate(".rdata$engine")) const unsigned long long engine_table_sentinel =
    0x0102030405060708ULL;
extern "C" __declspec(dllexport) __declspec(allocate(".rdata$engine")) const char* const engine_pointer_data[] = {
    engine_banner,
    engine_format,
    engine_short,
};

/// Adds two integral values through a standalone overload.
extern "C" __declspec(dllexport) __declspec(noinline) int overloaded_sum(int left, int right) {
    return left + right + 0x21;
}

/// Adds two floating-point values through the second standalone overload.
extern "C" __declspec(dllexport) __declspec(noinline) float overloaded_sum_float(float left, float right) {
    return left * 1.5f + right * 0.5f;
}

/// Computes a recursive score with a stable base case.
extern "C" __declspec(dllexport) __declspec(noinline) int recursive_score(int depth) {
    if (depth <= 1)
        return 1;
    return depth + recursive_score(depth - 1);
}

/// Starts one half of the mutually recursive pair.
extern "C" __declspec(dllexport) __declspec(noinline) int mutual_alpha(int depth);

/// Starts the second half of the mutually recursive pair.
extern "C" __declspec(dllexport) __declspec(noinline) int mutual_beta(int depth);

/// Completes the mutually recursive alpha branch.
int mutual_alpha(int depth) {
    if (depth <= 0)
        return 2;
    return mutual_beta(depth - 1) + 1;
}

/// Completes the mutually recursive beta branch.
int mutual_beta(int depth) {
    if (depth <= 0)
        return 3;
    return mutual_alpha(depth - 1) + 2;
}

/// Implements a dense switch that naturally encourages a compiler jump table.
extern "C" __declspec(dllexport) __declspec(noinline) int switch_mode(int mode) {
    switch (mode) {
        case 0:
            return 10;
        case 1:
            return 20;
        case 2:
            return 30;
        case 3:
            return 40;
        case 4:
            return 50;
        case 5:
            return 60;
        case 6:
            return 70;
        case 7:
            return 80;
        default:
            return -1;
    }
}

/// Implements a sparse switch negative control for jump-table recovery.
extern "C" __declspec(dllexport) __declspec(noinline) int sparse_mode(int mode) {
    switch (mode) {
        case -100:
            return 1;
        case 77:
            return 2;
        case 1000:
            return 3;
        default:
            return 0;
    }
}

/// Invokes one callback table entry through an indirect call.
extern "C" __declspec(dllexport) __declspec(noinline) unsigned int invoke_callback(unsigned int index,
                                                                                   unsigned int value) {
    const EngineCallback callback = engine_callbacks[index & 3U];
    return callback(value);
}

/// Mutates an entity through a reference and a virtual update call.
extern "C" __declspec(dllexport) __declspec(noinline) int update_entity(engine::Entity& entity, int tick) {
    const int value = entity.update(tick);
    engine_state ^= static_cast<unsigned int>(entity.kind()[0]);
    return value;
}

/// Updates an entity through a pointer and a nested direct member call.
extern "C" __declspec(dllexport) __declspec(noinline) int update_entity_pointer(engine::Entity* entity, int tick) {
    return entity == nullptr ? -1 : update_entity(*entity, tick + 1);
}

/// Calls a no-return engine path under an adversarial sentinel branch.
extern "C" __declspec(dllexport) __declspec(noinline) __declspec(noreturn) void engine_abort() {
    for (;;) {
        engine_state ^= 0xdeadbeefu;
    }
}

/// Provides a no-return caller whose normal branch remains analyzable.
extern "C" __declspec(dllexport) __declspec(noinline) void abort_path_one() {
    if (engine_selector == -1)
        engine_abort();
    engine_state += 1U;
}

/// Provides a second no-return caller with a different condition.
extern "C" __declspec(dllexport) __declspec(noinline) void abort_path_two() {
    if (engine_selector == -2)
        engine_abort();
    engine_state += 2U;
}

/// Provides a third no-return caller to supply repeated no-return evidence.
extern "C" __declspec(dllexport) __declspec(noinline) void abort_path_three() {
    if (engine_selector == -3)
        engine_abort();
    engine_state += 3U;
}

/// Calls the imported Windows resource API with constant string-table IDs.
extern "C" __declspec(dllexport) __declspec(noinline) void resource_lookup() {
    wchar_t buffer[64] = {};
    engine_state = static_cast<unsigned int>(LoadStringW(nullptr, IDS_GREETING, buffer, 64));
    engine_state += static_cast<unsigned int>(LoadStringW(nullptr, IDS_SECOND, buffer, 64));
}

/// Calls imported runtime APIs and keeps external references in the call graph.
extern "C" __declspec(dllexport) __declspec(noinline) void system_calls() {
    OutputDebugStringA(engine_banner);
    engine_global_ticks += static_cast<unsigned long long>(GetTickCount());
}

/// Provides a no-return wrapper around the imported process termination API.
extern "C" __declspec(dllexport) __declspec(noinline) __declspec(noreturn) void shutdown_engine() {
    ExitProcess(engine_state);
}

/// Runs the full object, arithmetic, stack, switch, callback, and reference mix.
extern "C" __declspec(dllexport) __declspec(noinline) unsigned int engine_tick(int tick, const char* label) {
    volatile unsigned int stack_value = static_cast<unsigned int>(tick) ^ 0x55aaU;
    volatile unsigned long long stack_wide = engine_global_ticks + stack_value;
    volatile unsigned char stack_byte = static_cast<unsigned char>(stack_value);
    engine::Vehicle vehicle{11, label, 90};
    engine::Player player{22, label, 100, 0x1234U};
    engine::TextureAsset texture{0x7001U, 64U, 32U};
    engine::ScriptAsset script{0x7002U, 4096U};
    demangle_fixture::Calculator calculator;
    engine::Entity* entities[2] = {&vehicle, &player};
    const int virtual_score = update_entity_pointer(entities[tick & 1], tick);
    vehicle.refuel(5);
    player.select_weapon(0x5678U);
    player.damage(tick & 3);
    const unsigned int resource_score = texture.size() + script.size() + script.checksum(stack_value);
    const unsigned int callback_score = invoke_callback(static_cast<unsigned int>(tick), stack_value);
    const int calculator_score = calculator.add(tick, 5) + demangle_fixture::Calculator::scale(2);
    const int branch_score = switch_mode(engine_selector) + sparse_mode(engine_selector);
    const int recursion_score = recursive_score((tick & 3) + 2) + mutual_alpha(tick & 3);
    const float floating_score = overloaded_sum_float(static_cast<float>(tick), 2.0f);
    const unsigned int result =
        static_cast<unsigned int>(overloaded_sum(virtual_score, branch_score) + recursion_score + calculator_score +
                                  overload_fixture::combine(tick, 3)) +
        callback_score + resource_score + static_cast<unsigned int>(floating_score) +
        static_cast<unsigned int>(stack_wide) + stack_byte;
    engine_static_cache ^= result;
    engine_global_ticks += result;
    return result;
}

/// Holds an exported but unreachable routine as an intentional dead-code control.
extern "C" __declspec(dllexport) __declspec(noinline) int engine_unused_dead_code(int value) {
    return value * 0x7f + 0x123;
}

// Separate code segment ordering creates genuine linker filler and alignment
// boundaries for Condense Filler Bytes without hand-written assembly.
#pragma code_seg(push, engine_filler_code, ".text$F")

/// Adds one repeated function boundary for filler-byte analysis.
extern "C" __declspec(dllexport) __declspec(noinline) void filler_engine_a() {
    engine_state += 0x11U;
}

/// Adds a second repeated function boundary for filler-byte analysis.
extern "C" __declspec(dllexport) __declspec(noinline) void filler_engine_b() {
    engine_state += 0x22U;
}

/// Adds a third repeated function boundary for filler-byte analysis.
extern "C" __declspec(dllexport) __declspec(noinline) void filler_engine_c() {
    engine_state += 0x33U;
}

/// Adds a fourth repeated function boundary for filler-byte analysis.
extern "C" __declspec(dllexport) __declspec(noinline) void filler_engine_d() {
    engine_state += 0x44U;
}

#pragma code_seg(pop, engine_filler_code)

/// Calls every major fixture path so normal flow discovery reaches the graph.
extern "C" __declspec(dllexport) __declspec(noinline) void fixture_entry() {
    filler_engine_a();
    filler_engine_b();
    filler_engine_c();
    filler_engine_d();
    resource_lookup();
    system_calls();
    abort_path_one();
    abort_path_two();
    abort_path_three();
    engine_state = engine_tick(engine_selector, engine_banner);
    for (unsigned int index = 0; index < 4U; ++index)
        engine_state ^= invoke_callback(index, engine_state);
    engine_global_ticks ^= engine_table_sentinel;
    engine_global_ticks ^= reinterpret_cast<unsigned long long>(engine_pointer_data[0]);
}
