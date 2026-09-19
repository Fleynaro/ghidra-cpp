// Deterministic MSVC corpus for custom P-Code emulation research.
// The ten exported functions are intentionally simple and noinline so Ghidra
// can identify each body and the Python harness can seed its calling state.

#include <cstdio>

#define RESEARCH_API extern "C" __declspec(dllexport) __declspec(noinline)

// Add two signed integers without introducing library calls.
RESEARCH_API int add(int x, int y) {
    return x + y;
}

// Accumulate a contiguous integer array through indexed loads.
RESEARCH_API int sum_array(const int* values, int count) {
    int total = 0;
    for (int index = 0; index < count; ++index) {
        total += values[index];
    }
    return total;
}

// Apply a scalar increment to every element of a mutable array.
RESEARCH_API void add_to_list(int* values, int count, int value) {
    for (int index = 0; index < count; ++index) {
        values[index] += value;
    }
}

// Multiply two integers through an explicit local result.
RESEARCH_API int multiply(int x, int y) {
    int product = x * y;
    return product;
}

// Return the non-negative distance between two signed integers.
RESEARCH_API int absolute_difference(int x, int y) {
    int difference = x - y;
    return difference < 0 ? -difference : difference;
}

// Count elements whose low bit is clear.
RESEARCH_API int count_even(const int* values, int count) {
    int result = 0;
    for (int index = 0; index < count; ++index) {
        if ((values[index] & 1) == 0) {
            ++result;
        }
    }
    return result;
}

// Select the nearest endpoint when a value lies outside an interval.
RESEARCH_API int clamp_value(int value, int low, int high) {
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

// Compute an integer dot product using two independently indexed arrays.
RESEARCH_API int dot_product(const int* left, const int* right, int count) {
    int total = 0;
    for (int index = 0; index < count; ++index) {
        total += left[index] * right[index];
    }
    return total;
}

// Reverse a contiguous array in place using symmetric swaps.
RESEARCH_API void reverse_in_place(int* values, int count) {
    for (int left = 0; left < count / 2; ++left) {
        const int right = count - left - 1;
        const int temporary = values[left];
        values[left] = values[right];
        values[right] = temporary;
    }
}

// Select one of two values using a concrete conditional branch.
RESEARCH_API int choose_value(int condition, int when_true, int when_false) {
    if (condition != 0) {
        return when_true;
    }
    return when_false;
}

// Run the same deterministic cases used by the PyGhidra harness normally.
int main() {
    int values[] = {1, 2, 3, 4};
    int additions[] = {10, 20, 30, 40};
    int dot_left[] = {1, 2, 3};
    int dot_right[] = {4, 5, 6};
    int reverse_values[] = {10, 20, 30, 40};
    std::printf("add=%d\n", add(10, 32));
    std::printf("sum_array=%d\n", sum_array(values, 4));
    add_to_list(additions, 4, 5);
    std::printf("add_to_list=%d,%d,%d,%d\n", additions[0], additions[1], additions[2], additions[3]);
    std::printf("multiply=%d\n", multiply(7, 6));
    std::printf("absolute_difference=%d\n", absolute_difference(9, 23));
    std::printf("count_even=%d\n", count_even(values, 4));
    std::printf("clamp_value=%d\n", clamp_value(99, 0, 50));
    std::printf("dot_product=%d\n", dot_product(dot_left, dot_right, 3));
    reverse_in_place(reverse_values, 4);
    std::printf("reverse_in_place=%d,%d,%d,%d\n", reverse_values[0], reverse_values[1], reverse_values[2], reverse_values[3]);
    std::printf("choose_value=%d\n", choose_value(1, 11, 22));
    return 0;
}
