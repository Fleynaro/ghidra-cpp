void func_1(int *param_1, int param_2, int param_3) {
    if (param_2 == 0) {
        return;
    }
    param_3 %= param_2;
    if (param_3 < 0) {
        param_3 += param_2;
    }
    int local_1 = 0;
    int local_2 = param_3 - 1;
    while (local_1 < local_2) {
        int local_3 = param_1[local_1];
        param_1[local_1] = param_1[local_2];
        param_1[local_2] = local_3;
        ++local_1;
        --local_2;
    }
    local_1 = param_3;
    local_2 = param_2 - 1;
    while (local_1 < local_2) {
        int local_4 = param_1[local_1];
        param_1[local_1] = param_1[local_2];
        param_1[local_2] = local_4;
        ++local_1;
        --local_2;
    }
    local_1 = 0;
    local_2 = param_2 - 1;
    while (local_1 < local_2) {
        int local_5 = param_1[local_1];
        param_1[local_1] = param_1[local_2];
        param_1[local_2] = local_5;
        ++local_1;
        --local_2;
    }
}
