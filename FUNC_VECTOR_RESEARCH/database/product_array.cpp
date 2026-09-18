long long func_1(const int *param_1, int param_2) {
    long long local_1 = 1;
    for (int local_2 = 0; local_2 < param_2; ++local_2) {
        local_1 *= param_1[local_2];
    }
    return local_1;
}
