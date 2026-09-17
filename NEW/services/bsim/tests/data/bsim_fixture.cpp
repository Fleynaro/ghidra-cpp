// ASCII-only BSim fixture with deliberately separate utility functions.
//
// The paired functions implement the same contract through different control
// flow so signature comparison can identify semantic twins rather than merely
// duplicate source text. Every utility is externally visible and no-inline so
// the generated PE retains stable standalone function boundaries.

// Sorts an integer array in ascending order with insertion sort.
extern "C" __declspec(noinline) void sort_insertion_ascending(int* values, int count) {
    if (values == 0 || count < 2) {
        return;
    }

    for (int index = 1; index < count; ++index) {
        const int item = values[index];
        int position = index;
        while (position > 0 && values[position - 1] > item) {
            values[position] = values[position - 1];
            --position;
        }
        values[position] = item;
    }
}

// Sorts an integer array in ascending order with selection sort.
extern "C" __declspec(noinline) void sort_selection_ascending(int* values, int count) {
    if (values == 0 || count < 2) {
        return;
    }

    for (int index = 0; index < count - 1; ++index) {
        int smallest = index;
        for (int candidate = index + 1; candidate < count; ++candidate) {
            if (values[candidate] < values[smallest]) {
                smallest = candidate;
            }
        }
        if (smallest != index) {
            const int item = values[index];
            values[index] = values[smallest];
            values[smallest] = item;
        }
    }
}

// Returns the first index containing target, or -1 when the value is absent.
extern "C" __declspec(noinline) int find_first_linear(const int* values, int count, int target) {
    if (values == 0 || count <= 0) {
        return -1;
    }

    for (int index = 0; index < count; ++index) {
        if (values[index] == target) {
            return index;
        }
    }
    return -1;
}

// Returns the first index containing target by scanning from the end.
extern "C" __declspec(noinline) int find_first_reverse(const int* values, int count, int target) {
    if (values == 0 || count <= 0) {
        return -1;
    }

    int first = -1;
    for (int index = count - 1; index >= 0; --index) {
        if (values[index] == target) {
            first = index;
        }
    }
    return first;
}

// Returns the largest array element, or zero for an empty or null array.
extern "C" __declspec(noinline) int maximum_scan(const int* values, int count) {
    if (values == 0 || count <= 0) {
        return 0;
    }

    int largest = values[0];
    for (int index = 1; index < count; ++index) {
        if (values[index] > largest) {
            largest = values[index];
        }
    }
    return largest;
}

// Returns the largest array element by reducing adjacent pairs.
extern "C" __declspec(noinline) int maximum_pairwise(const int* values, int count) {
    if (values == 0 || count <= 0) {
        return 0;
    }

    int largest = values[0];
    int index = 1;
    for (; index + 1 < count; index += 2) {
        const int pair_largest = values[index] > values[index + 1] ? values[index] : values[index + 1];
        if (pair_largest > largest) {
            largest = pair_largest;
        }
    }
    if (index < count && values[index] > largest) {
        largest = values[index];
    }
    return largest;
}

// Returns the non-negative magnitude of an integer with minimum-value saturation.
extern "C" __declspec(noinline) int absolute_branch(int value) {
    const int minimum = -2147483647 - 1;
    if (value == minimum) {
        return 2147483647;
    }
    return value < 0 ? -value : value;
}

// Returns the non-negative magnitude using sign-bit arithmetic with saturation.
extern "C" __declspec(noinline) int absolute_mask(int value) {
    const int minimum = -2147483647 - 1;
    if (value == minimum) {
        return 2147483647;
    }

    const unsigned int bits = static_cast<unsigned int>(value);
    const unsigned int mask = bits >> 31U;
    const unsigned int magnitude = (bits ^ (0U - mask)) + mask;
    return static_cast<int>(magnitude);
}

// Computes an eight-bit additive checksum in forward byte order.
extern "C" __declspec(noinline) unsigned int checksum_forward(const unsigned char* bytes, int count) {
    if (bytes == 0 || count <= 0) {
        return 0U;
    }

    unsigned int checksum = 0U;
    for (int index = 0; index < count; ++index) {
        checksum += static_cast<unsigned int>(bytes[index]);
    }
    return checksum;
}

// Computes the same additive checksum by traversing bytes in reverse order.
extern "C" __declspec(noinline) unsigned int checksum_reverse(const unsigned char* bytes, int count) {
    if (bytes == 0 || count <= 0) {
        return 0U;
    }

    unsigned int checksum = 0U;
    for (int index = count - 1; index >= 0; --index) {
        checksum += static_cast<unsigned int>(bytes[index]);
    }
    return checksum;
}

// Computes the 32-bit FNV-1a hash through indexed byte access.
extern "C" __declspec(noinline) unsigned int hash_fnv_indexed(const unsigned char* bytes, int count) {
    if (bytes == 0 || count <= 0) {
        return 2166136261U;
    }

    unsigned int hash = 2166136261U;
    for (int index = 0; index < count; ++index) {
        hash ^= static_cast<unsigned int>(bytes[index]);
        hash *= 16777619U;
    }
    return hash;
}

// Computes the same 32-bit FNV-1a hash through pointer traversal.
extern "C" __declspec(noinline) unsigned int hash_fnv_pointer(const unsigned char* bytes, int count) {
    if (bytes == 0 || count <= 0) {
        return 2166136261U;
    }

    unsigned int hash = 2166136261U;
    const unsigned char* current = bytes;
    const unsigned char* end = bytes + count;
    while (current != end) {
        hash ^= static_cast<unsigned int>(*current);
        hash *= 16777619U;
        ++current;
    }
    return hash;
}

// Clamps value into the inclusive interval, accepting bounds in either order.
extern "C" __declspec(noinline) int clamp_integer(int value, int low, int high) {
    int lower = low;
    int upper = high;
    if (lower > upper) {
        const int swapped = lower;
        lower = upper;
        upper = swapped;
    }
    if (value < lower) {
        return lower;
    }
    if (value > upper) {
        return upper;
    }
    return value;
}

// Computes the truncated arithmetic mean of an integer array.
extern "C" __declspec(noinline) int average_integer(const int* values, int count) {
    if (values == 0 || count <= 0) {
        return 0;
    }

    long long total = 0;
    for (int index = 0; index < count; ++index) {
        total += static_cast<long long>(values[index]);
    }
    return static_cast<int>(total / static_cast<long long>(count));
}

// Counts non-zero elements in an integer array.
extern "C" __declspec(noinline) int count_nonzero(const int* values, int count) {
    if (values == 0 || count <= 0) {
        return 0;
    }

    int total = 0;
    for (int index = 0; index < count; ++index) {
        if (values[index] != 0) {
            ++total;
        }
    }
    return total;
}

// Computes the wrapped sum of squared integer elements.
extern "C" __declspec(noinline) unsigned int sum_squares(const int* values, int count) {
    if (values == 0 || count <= 0) {
        return 0U;
    }

    unsigned int total = 0U;
    for (int index = 0; index < count; ++index) {
        const long long item = static_cast<long long>(values[index]);
        total += static_cast<unsigned int>(item * item);
    }
    return total;
}

// Computes the wrapped dot product of two integer arrays.
extern "C" __declspec(noinline) long long dot_product(const int* left, const int* right, int count) {
    if (left == 0 || right == 0 || count <= 0) {
        return 0;
    }

    long long total = 0;
    for (int index = 0; index < count; ++index) {
        total += static_cast<long long>(left[index]) * static_cast<long long>(right[index]);
    }
    return total;
}

// Rotates a 32-bit value left by a count reduced modulo 32.
extern "C" __declspec(noinline) unsigned int rotate_left32(unsigned int value, unsigned int bits) {
    const unsigned int amount = bits & 31U;
    if (amount == 0U) {
        return value;
    }
    return (value << amount) | (value >> (32U - amount));
}

// Counts set bits in a 32-bit value using repeated low-bit removal.
extern "C" __declspec(noinline) unsigned int population_count(unsigned int value) {
    unsigned int total = 0U;
    while (value != 0U) {
        value &= value - 1U;
        ++total;
    }
    return total;
}

// Reports whether value is a non-zero power of two.
extern "C" __declspec(noinline) int is_power_of_two(unsigned int value) {
    return value != 0U && (value & (value - 1U)) == 0U ? 1 : 0;
}

// Returns the next power of two, or zero when it cannot fit in 32 bits.
extern "C" __declspec(noinline) unsigned int next_power_of_two(unsigned int value) {
    if (value == 0U) {
        return 1U;
    }
    if (value > 0x80000000U) {
        return 0U;
    }

    unsigned int result = 1U;
    while (result < value) {
        result <<= 1U;
    }
    return result;
}

// Computes the greatest common divisor of two unsigned integers.
extern "C" __declspec(noinline) unsigned int gcd_unsigned(unsigned int left, unsigned int right) {
    while (right != 0U) {
        const unsigned int remainder = left % right;
        left = right;
        right = remainder;
    }
    return left;
}

// Computes a Fibonacci number modulo modulus with constant storage.
extern "C" __declspec(noinline) unsigned int fibonacci_modulo(unsigned int index, unsigned int modulus) {
    if (modulus == 0U) {
        return 0U;
    }
    if (index == 0U) {
        return 0U;
    }

    unsigned int previous = 0U;
    unsigned int current = 1U % modulus;
    for (unsigned int position = 1U; position < index; ++position) {
        const unsigned int next = (previous + current) % modulus;
        previous = current;
        current = next;
    }
    return current;
}

// Computes the triangular number for value using widened intermediate arithmetic.
extern "C" __declspec(noinline) unsigned long long triangular_number(unsigned int value) {
    return static_cast<unsigned long long>(value) * static_cast<unsigned long long>(value + 1U) / 2ULL;
}

// Reverses the decimal digits of an unsigned integer.
extern "C" __declspec(noinline) unsigned int reverse_decimal_digits(unsigned int value) {
    unsigned int reversed = 0U;
    while (value != 0U) {
        reversed = reversed * 10U + value % 10U;
        value /= 10U;
    }
    return reversed;
}

// Swaps the byte order of a 32-bit value.
extern "C" __declspec(noinline) unsigned int byte_swap32(unsigned int value) {
    return ((value & 0x000000ffU) << 24U) | ((value & 0x0000ff00U) << 8U) | ((value & 0x00ff0000U) >> 8U) |
           ((value & 0xff000000U) >> 24U);
}

// Reports whether a byte is an ASCII printable character.
extern "C" __declspec(noinline) int is_ascii_printable(unsigned char value) {
    return value >= 32U && value <= 126U ? 1 : 0;
}

// Counts contiguous runs containing target in a byte array.
extern "C" __declspec(noinline) int count_byte_runs(const unsigned char* bytes, int count, unsigned char target) {
    if (bytes == 0 || count <= 0) {
        return 0;
    }

    int runs = 0;
    int inside = 0;
    for (int index = 0; index < count; ++index) {
        if (bytes[index] == target) {
            if (!inside) {
                ++runs;
                inside = 1;
            }
        } else {
            inside = 0;
        }
    }
    return runs;
}

// Packs two low nibbles into one byte, placing high_nibble first.
extern "C" __declspec(noinline) unsigned char pack_nibbles(unsigned char high_nibble, unsigned char low_nibble) {
    return static_cast<unsigned char>(((high_nibble & 0x0fU) << 4U) | (low_nibble & 0x0fU));
}

// Computes the midpoint of an ordered unsigned interval without overflow.
extern "C" __declspec(noinline) unsigned int midpoint_unsigned(unsigned int low, unsigned int high) {
    if (low > high) {
        const unsigned int swapped = low;
        low = high;
        high = swapped;
    }
    return low + (high - low) / 2U;
}

// Calls every utility so the linker retains all 30 standalone fixture functions.
int main() {
    int first[] = {7, -2, 7, 4, 0, 9};
    int second[] = {7, -2, 7, 4, 0, 9};
    int values[] = {7, -2, 7, 4, 0, 9};
    int other[] = {3, 5, 2, 8, 1, 4};
    const unsigned char bytes[] = {1U, 2U, 3U, 2U, 1U, 0U, 2U};
    volatile long long sink = 0;

    sort_insertion_ascending(first, 6);
    sort_selection_ascending(second, 6);
    sink += static_cast<long long>(first[0] + second[0]);
    sink += static_cast<long long>(find_first_linear(values, 6, 7));
    sink += static_cast<long long>(find_first_reverse(values, 6, 7));
    sink += static_cast<long long>(maximum_scan(values, 6));
    sink += static_cast<long long>(maximum_pairwise(values, 6));
    sink += static_cast<long long>(absolute_branch(-23));
    sink += static_cast<long long>(absolute_mask(-23));
    sink += static_cast<long long>(checksum_forward(bytes, 7));
    sink += static_cast<long long>(checksum_reverse(bytes, 7));
    sink += static_cast<long long>(hash_fnv_indexed(bytes, 7));
    sink += static_cast<long long>(hash_fnv_pointer(bytes, 7));
    sink += static_cast<long long>(clamp_integer(12, 1, 9));
    sink += static_cast<long long>(average_integer(values, 6));
    sink += static_cast<long long>(count_nonzero(values, 6));
    sink += static_cast<long long>(sum_squares(values, 6));
    sink += dot_product(values, other, 6);
    sink += static_cast<long long>(rotate_left32(0x12345678U, 7U));
    sink += static_cast<long long>(population_count(0xf0f0f00fU));
    sink += static_cast<long long>(is_power_of_two(1024U));
    sink += static_cast<long long>(next_power_of_two(1000U));
    sink += static_cast<long long>(gcd_unsigned(84U, 30U));
    sink += static_cast<long long>(fibonacci_modulo(17U, 1000U));
    sink += static_cast<long long>(triangular_number(20U));
    sink += static_cast<long long>(reverse_decimal_digits(12030U));
    sink += static_cast<long long>(byte_swap32(0x10203040U));
    sink += static_cast<long long>(is_ascii_printable(static_cast<unsigned char>('A')));
    sink += static_cast<long long>(count_byte_runs(bytes, 7, static_cast<unsigned char>(2U)));
    sink += static_cast<long long>(pack_nibbles(0x0aU, 0x05U));
    sink += static_cast<long long>(midpoint_unsigned(10U, 20U));

    return sink == -1 ? 1 : 0;
}
#if defined(_MSC_VER)
#pragma comment(linker, "/EXPORT:sort_insertion_ascending")
#pragma comment(linker, "/EXPORT:sort_selection_ascending")
#pragma comment(linker, "/EXPORT:find_first_linear")
#pragma comment(linker, "/EXPORT:find_first_reverse")
#pragma comment(linker, "/EXPORT:maximum_scan")
#pragma comment(linker, "/EXPORT:maximum_pairwise")
#pragma comment(linker, "/EXPORT:absolute_branch")
#pragma comment(linker, "/EXPORT:absolute_mask")
#pragma comment(linker, "/EXPORT:checksum_forward")
#pragma comment(linker, "/EXPORT:checksum_reverse")
#pragma comment(linker, "/EXPORT:hash_fnv_indexed")
#pragma comment(linker, "/EXPORT:hash_fnv_pointer")
#pragma comment(linker, "/EXPORT:clamp_integer")
#pragma comment(linker, "/EXPORT:average_integer")
#pragma comment(linker, "/EXPORT:count_nonzero")
#pragma comment(linker, "/EXPORT:sum_squares")
#pragma comment(linker, "/EXPORT:dot_product")
#pragma comment(linker, "/EXPORT:rotate_left32")
#pragma comment(linker, "/EXPORT:population_count")
#pragma comment(linker, "/EXPORT:is_power_of_two")
#pragma comment(linker, "/EXPORT:next_power_of_two")
#pragma comment(linker, "/EXPORT:gcd_unsigned")
#pragma comment(linker, "/EXPORT:fibonacci_modulo")
#pragma comment(linker, "/EXPORT:triangular_number")
#pragma comment(linker, "/EXPORT:reverse_decimal_digits")
#pragma comment(linker, "/EXPORT:byte_swap32")
#pragma comment(linker, "/EXPORT:is_ascii_printable")
#pragma comment(linker, "/EXPORT:count_byte_runs")
#pragma comment(linker, "/EXPORT:pack_nibbles")
#pragma comment(linker, "/EXPORT:midpoint_unsigned")
#endif
