void func_1(int *param_1, int param_2) {
    int local_1 = 0;
    for (int local_2 = 0; local_2 < param_2; ++local_2) {
        local_1 += param_1[local_2];
        param_1[local_2] = local_1;
    }
}
