double func_1(const int *param_1, int param_2) {
    if (param_2 == 0) {
        return 0.0;
    }
    long long local_1 = 0;
    for (int local_2 = 0; local_2 < param_2; ++local_2) {
        local_1 += param_1[local_2];
    }
    return static_cast<double>(local_1) / param_2;
}
