int func_1(const int *param_1, int param_2, int param_3) {
    int local_1 = 0;
    for (int local_2 = 0; local_2 < param_2; ++local_2) {
        if (param_1[local_2] == param_3) {
            ++local_1;
        }
    }
    return local_1;
}
