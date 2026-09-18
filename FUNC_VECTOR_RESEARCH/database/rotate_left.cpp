unsigned int func_1(unsigned int param_1, unsigned int param_2) {
    param_2 &= 31;
    if (param_2 == 0) {
        return param_1;
    }
    return (param_1 << param_2) | (param_1 >> (32 - param_2));
}
