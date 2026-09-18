int func_1(const char *param_1) {
    int local_1 = 1;
    int local_2 = 0;
    while (*param_1 == ' ' || *param_1 == '\t' || *param_1 == '\n') {
        ++param_1;
    }
    if (*param_1 == '-' || *param_1 == '+') {
        local_1 = *param_1 == '-' ? -1 : 1;
        ++param_1;
    }
    while (*param_1 >= '0' && *param_1 <= '9') {
        local_2 = local_2 * 10 + (*param_1 - '0');
        ++param_1;
    }
    return local_1 * local_2;
}
