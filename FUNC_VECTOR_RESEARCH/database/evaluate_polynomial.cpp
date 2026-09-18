double func_1(const double *param_1, int param_2, double param_3) {
    double local_1 = 0.0;
    for (int local_2 = param_2 - 1; local_2 >= 0; --local_2) {
        local_1 = local_1 * param_3 + param_1[local_2];
    }
    return local_1;
}
