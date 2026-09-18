bool func_1(const int *param_1, int param_2) {
    for (int local_1 = 1; local_1 < param_2; ++local_1) {
        if (param_1[local_1] < param_1[local_1 - 1]) {
            return false;
        }
    }
    return true;
}
