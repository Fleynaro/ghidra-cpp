long long func_1(const int *param_1, const int *param_2, int param_3) {
    long long local_1 = 0;
    for (int local_2 = 0; local_2 < param_3; ++local_2) {
        local_1 += static_cast<long long>(param_1[local_2]) * param_2[local_2];
    }
    return local_1;
}
