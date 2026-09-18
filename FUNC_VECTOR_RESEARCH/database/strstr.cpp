const char *func_1(const char *param_1, const char *param_2) {
    if (param_2[0] == 0) {
        return param_1;
    }
    for (int local_1 = 0; param_1[local_1] != 0; ++local_1) {
        int local_2 = 0;
        while (param_1[local_1 + local_2] != 0 && param_2[local_2] != 0 && param_1[local_1 + local_2] == param_2[local_2]) {
            ++local_2;
        }
        if (param_2[local_2] == 0) {
            return param_1 + local_1;
        }
    }
    return nullptr;
}
