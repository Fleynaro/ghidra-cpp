bool func_1(unsigned int param_1) {
    if (param_1 < 2) {
        return false;
    }
    for (unsigned int local_1 = 2; local_1 <= param_1 / local_1; ++local_1) {
        if (param_1 % local_1 == 0) {
            return false;
        }
    }
    return true;
}
