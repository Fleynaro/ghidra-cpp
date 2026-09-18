double func_1(double param_1) {
    if (param_1 <= 0.0) {
        return 0.0;
    }
    double local_1 = param_1 > 1.0 ? param_1 : 1.0;
    double local_2 = 0.0;
    while (local_1 != local_2) {
        local_2 = local_1;
        local_1 = 0.5 * (local_1 + param_1 / local_1);
        if (local_1 - local_2 < 0.000000001 && local_2 - local_1 < 0.000000001) {
            break;
        }
    }
    return local_1;
}
