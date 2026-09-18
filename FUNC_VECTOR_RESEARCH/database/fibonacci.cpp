unsigned long long func_1(unsigned int param_1) {
    unsigned long long local_1 = 0;
    unsigned long long local_2 = 1;
    for (unsigned int local_3 = 0; local_3 < param_1; ++local_3) {
        unsigned long long local_4 = local_1 + local_2;
        local_1 = local_2;
        local_2 = local_4;
    }
    return local_1;
}
