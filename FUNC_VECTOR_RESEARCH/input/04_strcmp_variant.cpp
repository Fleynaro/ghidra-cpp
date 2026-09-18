int func_1(const char *param_1, const char *param_2) {
    while (*param_1 && *param_1 == *param_2) {
        ++param_1;
        ++param_2;
    }
    if (static_cast<unsigned char>(*param_1) < static_cast<unsigned char>(*param_2)) {
        return -1;
    }
    if (static_cast<unsigned char>(*param_1) > static_cast<unsigned char>(*param_2)) {
        return 1;
    }
    return 0;
}
