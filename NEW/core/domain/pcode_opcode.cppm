export module recode.core.pcode_opcode;

import std;

export namespace recode::core {

/// Stable public p-code opcode mapping; unknown values remain serializable.
enum class PcodeOpcode : std::uint16_t {
    unknown = 0,
    copy = 1,
    load = 2,
    store = 3,
    branch = 4,
    cbranch = 5,
    branch_ind = 6,
    call = 7,
    call_ind = 8,
    call_other = 9,
    return_op = 10,
    int_equal = 11,
    int_not_equal = 12,
    int_sless = 13,
    int_sless_equal = 14,
    int_less = 15,
    int_less_equal = 16,
    int_zext = 17,
    int_sext = 18,
    int_add = 19,
    int_sub = 20,
    int_carry = 21,
    int_scarry = 22,
    int_sborrow = 23,
    int_two_comp = 24,
    int_negate = 25,
    int_xor = 26,
    int_and = 27,
    int_or = 28,
    int_left = 29,
    int_right = 30,
    int_sright = 31,
    int_mult = 32,
    int_div = 33,
    int_sdiv = 34,
    int_rem = 35,
    int_srem = 36,
    bool_negate = 37,
    bool_xor = 38,
    bool_and = 39,
    bool_or = 40,
    float_equal = 41,
    float_not_equal = 42,
    float_less = 43,
    float_less_equal = 44,
    float_nan = 46,
    float_add = 47,
    float_div = 48,
    float_mult = 49,
    float_sub = 50,
    float_neg = 51,
    float_abs = 52,
    float_sqrt = 53,
    float_int_to_float = 54,
    float_float_to_float = 55,
    float_trunc = 56,
    float_ceil = 57,
    float_floor = 58,
    float_round = 59,
    multiequal = 60,
    indirect = 61,
    piece = 62,
    subpiece = 63,
    cast = 64,
    ptradd = 65,
    ptrsub = 66,
    segment_op = 67,
    cpool_ref = 68,
    new_op = 69,
    insert = 70,
    zpull = 71,
    popcount = 72,
    lzcount = 73,
    spull = 74,
};

/// Returns the stable textual spelling of an opcode.
[[nodiscard]] inline std::string_view pcode_opcode_name(PcodeOpcode opcode) noexcept {
    switch (opcode) {
        case PcodeOpcode::copy:
            return "COPY";
        case PcodeOpcode::load:
            return "LOAD";
        case PcodeOpcode::store:
            return "STORE";
        case PcodeOpcode::branch:
            return "BRANCH";
        case PcodeOpcode::cbranch:
            return "CBRANCH";
        case PcodeOpcode::call:
            return "CALL";
        case PcodeOpcode::call_ind:
            return "CALLIND";
        case PcodeOpcode::return_op:
            return "RETURN";
        default:
            return "UNKNOWN";
    }
}

} // namespace recode::core
