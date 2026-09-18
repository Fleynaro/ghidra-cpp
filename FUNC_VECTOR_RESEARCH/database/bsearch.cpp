int func_1(const int *param_1, int param_2, int param_3) {
    int local_1 = 0;
    int local_2 = param_2 - 1;
    while (local_1 <= local_2) {
        int local_3 = local_1 + (local_2 - local_1) / 2;
        if (param_1[local_3] == param_3) {
            return local_3;
        }
        if (param_1[local_3] < param_3) {
            local_1 = local_3 + 1;
        } else {
            local_2 = local_3 - 1;
        }
    }
    return -1;
}
