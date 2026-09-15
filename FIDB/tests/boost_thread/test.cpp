// Independent Boost.Thread consumer for the Ghidra FID pipeline.

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include <cstdint>

namespace {

// Exercise a real worker thread, mutex, condition variable, and join operation.
__declspec(noinline) std::uint32_t exercise_thread() {
    boost::mutex mutex;
    boost::condition_variable condition;
    bool ready = false;
    std::uint32_t value = 0;
    boost::thread worker([&] {
        boost::unique_lock<boost::mutex> lock(mutex);
        value = 42U;
        ready = true;
        condition.notify_one();
    });
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        condition.wait(lock, [&] { return ready; });
    }
    worker.join();
    return value;
}

// This function is not supplied by Boost and is the negative FID control.
__declspec(noinline) std::uint32_t non_boost_control(std::uint32_t value) {
    value ^= 0x10203040U;
    return value * 214013U + 2531011U;
}

}  // namespace

// Make both paths part of the executable's observable result.
int main() {
    const std::uint32_t result = exercise_thread();
    const std::uint32_t control = non_boost_control(result);
    return result == 0U || control == 0U ? 1 : 0;
}
