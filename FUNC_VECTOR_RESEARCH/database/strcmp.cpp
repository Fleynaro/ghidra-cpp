int func_1(const char *param_1, const char *param_2) {
    int local_1 = 0;
    while (param_1[local_1] != 0 && param_1[local_1] == param_2[local_1]) {
        ++local_1;
    }
    return static_cast<unsigned char>(param_1[local_1]) - static_cast<unsigned char>(param_2[local_1]);
}
