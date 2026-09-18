int func_1(int *param_1, int param_2, int param_3, int param_4) {
    int local_1 = param_2;
    for (int local_2 = param_2; local_2 < param_3; ++local_2) {
        if (param_1[local_2] < param_4) {
            int local_3 = param_1[local_1];
            param_1[local_1] = param_1[local_2];
            param_1[local_2] = local_3;
            ++local_1;
        }
    }
    return local_1;
}
