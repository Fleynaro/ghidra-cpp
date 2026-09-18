int func_1(int *param_1, int param_2) {
    if (param_2 == 0) {
        return 0;
    }
    int local_1 = 1;
    for (int local_2 = 1; local_2 < param_2; ++local_2) {
        if (param_1[local_2] != param_1[local_1 - 1]) {
            param_1[local_1] = param_1[local_2];
            ++local_1;
        }
    }
    return local_1;
}
