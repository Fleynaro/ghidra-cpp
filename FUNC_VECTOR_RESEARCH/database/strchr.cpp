const char *func_1(const char *param_1, char param_2) {
    for (int local_1 = 0;; ++local_1) {
        if (param_1[local_1] == param_2) {
            return param_1 + local_1;
        }
        if (param_1[local_1] == 0) {
            return nullptr;
        }
    }
}
