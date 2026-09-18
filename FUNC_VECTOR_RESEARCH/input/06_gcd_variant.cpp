int func_1(int param_1, int param_2) {
    if (param_1 < 0) {
        param_1 = -param_1;
    }
    if (param_2 < 0) {
        param_2 = -param_2;
    }
    while (param_1 != param_2) {
        if (param_1 > param_2) {
            param_1 -= param_2;
        } else {
            param_2 -= param_1;
        }
    }
    return param_1;
}
