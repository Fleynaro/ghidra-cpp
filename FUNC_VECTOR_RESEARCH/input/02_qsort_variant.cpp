void func_1(int *param_1, int param_2) {
    for (int local_1 = 1; local_1 < param_2; ++local_1) {
        int local_2 = param_1[local_1];
        int local_3 = local_1;
        while (local_3 > 0 && param_1[local_3 - 1] > local_2) {
            param_1[local_3] = param_1[local_3 - 1];
            --local_3;
        }
        param_1[local_3] = local_2;
    }
}
