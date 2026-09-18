double func_1(double param_1, double param_2, double param_3, double param_4, double param_5) {
    if (param_2 == param_1) {
        return param_4;
    }
    double local_1 = (param_3 - param_1) / (param_2 - param_1);
    return param_4 + local_1 * (param_5 - param_4);
}
