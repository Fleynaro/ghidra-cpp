void func_1(int *param_1, int param_2, int param_3) {
    for (int local_1 = 0; local_1 < param_2; ++local_1) {
        param_1[local_1] = param_3 + local_1;
    }
}
