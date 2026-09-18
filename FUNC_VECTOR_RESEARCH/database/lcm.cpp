int func_1(int param_1, int param_2) {
    int local_1 = param_1 < 0 ? -param_1 : param_1;
    int local_2 = param_2 < 0 ? -param_2 : param_2;
    int local_3 = local_1;
    int local_4 = local_2;
    while (local_4 != 0) {
        int local_5 = local_3 % local_4;
        local_3 = local_4;
        local_4 = local_5;
    }
    return local_3 == 0 ? 0 : (local_1 / local_3) * local_2;
}
