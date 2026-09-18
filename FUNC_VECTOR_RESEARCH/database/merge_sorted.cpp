void func_1(const int *param_1, int param_2, const int *param_3, int param_4, int *param_5) {
    int local_1 = 0;
    int local_2 = 0;
    int local_3 = 0;
    while (local_1 < param_2 && local_2 < param_4) {
        if (param_1[local_1] <= param_3[local_2]) {
            param_5[local_3] = param_1[local_1];
            ++local_1;
        } else {
            param_5[local_3] = param_3[local_2];
            ++local_2;
        }
        ++local_3;
    }
    while (local_1 < param_2) {
        param_5[local_3] = param_1[local_1];
        ++local_1;
        ++local_3;
    }
    while (local_2 < param_4) {
        param_5[local_3] = param_3[local_2];
        ++local_2;
        ++local_3;
    }
}
