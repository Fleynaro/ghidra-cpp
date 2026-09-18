double func_1(double param_1, int param_2) {
    double local_1 = 1.0;
    int local_2 = param_2 < 0 ? -param_2 : param_2;
    while (local_2 > 0) {
        if ((local_2 & 1) != 0) {
            local_1 *= param_1;
        }
        param_1 *= param_1;
        local_2 >>= 1;
    }
    return param_2 < 0 ? 1.0 / local_1 : local_1;
}
