void func_1(int *param_1, int param_2) {
    int local_1 = 0;
    int local_2 = param_2 - 1;
    while (local_1 < local_2) {
        int local_3 = param_1[local_1];
        param_1[local_1] = param_1[local_2];
        param_1[local_2] = local_3;
        ++local_1;
        --local_2;
    }
}
