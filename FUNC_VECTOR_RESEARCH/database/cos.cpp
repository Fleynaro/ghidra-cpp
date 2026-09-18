double func_1(double param_1) {
    double local_1 = 1.0;
    double local_2 = 1.0;
    double local_3 = param_1 * param_1;
    for (int local_4 = 1; local_4 <= 8; ++local_4) {
        local_2 = -local_2 * local_3 / ((2 * local_4 - 1) * (2 * local_4));
        local_1 += local_2;
    }
    return local_1;
}
