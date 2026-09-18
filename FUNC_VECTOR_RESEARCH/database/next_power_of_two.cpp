unsigned int func_1(unsigned int param_1) {
    if (param_1 <= 1) {
        return 1;
    }
    --param_1;
    param_1 |= param_1 >> 1;
    param_1 |= param_1 >> 2;
    param_1 |= param_1 >> 4;
    param_1 |= param_1 >> 8;
    param_1 |= param_1 >> 16;
    return param_1 + 1;
}
