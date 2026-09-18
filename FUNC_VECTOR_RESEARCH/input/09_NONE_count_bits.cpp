int func_1(unsigned int param_1) {
    int local_1 = 0;
    while (param_1 != 0) {
        param_1 &= param_1 - 1;
        ++local_1;
    }
    return local_1;
}
