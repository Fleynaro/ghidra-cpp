int func_1(const char *param_1) {
    const char *local_1 = param_1;
    while (*local_1) {
        ++local_1;
    }
    return static_cast<int>(local_1 - param_1);
}
