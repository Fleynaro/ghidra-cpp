// Real MSVC x64 debuggee for the generic debugger contract integration suite.
//
// The fixture intentionally uses normal CRT synchronization and a PDB.  The
// debugger tests inspect real process state rather than a synthetic trace.

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <windows.h>

// Used by debugger integration tests to verify reading and writing process memory.
extern "C" __declspec(dllexport) volatile std::uint64_t g_debug_value = 0x1122334455667788ULL;

// Written by the exception integration test to select the controlled fault path.
constexpr std::uint64_t g_exception_trigger_value = 0xCAFEBABECAFEBABEULL;

// Used to keep worker threads alive at a deterministic synchronization point.
std::mutex g_worker_mutex;
std::condition_variable g_worker_condition;
std::atomic<unsigned> g_ready_workers{};
std::atomic<bool> g_release_workers{};

// The debugger tests set a data breakpoint on this heap-backed storage.
std::unique_ptr<std::uint64_t> g_heap_value;

// This worker intentionally performs nested calls so stack-frame inspection and
// thread switching can be tested while several contexts are simultaneously live.
extern "C" __declspec(noinline) std::uint64_t worker_leaf(unsigned id) {
    g_debug_value ^= static_cast<std::uint64_t>(id + 1U);
    return g_debug_value;
}

// Adds a loop and a local variable so stepping and stack unwinding have useful frames.
extern "C" __declspec(noinline) std::uint64_t worker_nested(unsigned id) {
    std::uint64_t local = id;
    for (unsigned index = 0; index != 3U; ++index)
        local += worker_leaf(id + index);
    return local;
}

// Keeps each worker distinguishable while waiting on an explicit release signal.
extern "C" __declspec(noinline) void worker_thread(unsigned id) {
    const auto value = worker_nested(id);
    if (g_heap_value)
        *g_heap_value += value;
    g_ready_workers.fetch_add(1U, std::memory_order_release);
    g_worker_condition.notify_all();
    std::unique_lock<std::mutex> lock(g_worker_mutex);
    g_worker_condition.wait(lock, [] { return g_release_workers.load(std::memory_order_acquire); });
    // The post-wait write is a deterministic watchpoint target after resume.
    if (g_heap_value)
        *g_heap_value ^= static_cast<std::uint64_t>(id + 0x100U);
}

// Provides a stable multi-level call site for step-into and step-over tests.
extern "C" __declspec(noinline) std::uint64_t compute_value(std::uint64_t value) {
    const auto intermediate = worker_nested(static_cast<unsigned>(value & 3U));
    g_debug_value += intermediate + value;
    return g_debug_value;
}

// Contains deterministic heap, global, and loop data flow for breakpoint tests.
extern "C" __declspec(noinline) void debugger_test_entry() {
    std::uint64_t local = 7U;
    for (unsigned index = 0; index != 4U; ++index)
        local += compute_value(index);
    if (g_heap_value)
        *g_heap_value = local;
}

// Raises a controlled first-chance exception. Tests call this only after they
// have installed an exception expectation, so normal fixture startup is stable.
extern "C" __declspec(noinline) void trigger_controlled_exception() {
    RaiseException(0xE0424242U, EXCEPTION_NONCONTINUABLE, 0, nullptr);
}

int main() {
    g_heap_value = std::make_unique<std::uint64_t>(0xAABBCCDDEEFF0011ULL);
    std::vector<std::thread> workers;
    workers.reserve(3);
    for (unsigned id = 0; id != 3U; ++id)
        workers.emplace_back(worker_thread, id);

    {
        std::unique_lock<std::mutex> lock(g_worker_mutex);
        g_worker_condition.wait(lock, [] { return g_ready_workers.load(std::memory_order_acquire) == 3U; });
    }

    // This marker makes manual launches understandable and is not used as a
    // timing primitive by the tests.
    std::cout << "DEBUGGER_READY" << std::endl;
    debugger_test_entry();
    if (g_debug_value == g_exception_trigger_value)
        trigger_controlled_exception();

    g_release_workers.store(true, std::memory_order_release);
    g_worker_condition.notify_all();
    for (auto& worker : workers)
        worker.join();
    return static_cast<int>(g_debug_value & 0x7fU);
}
