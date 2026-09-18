void func_1(int *param_1, int param_2, int param_3) {
    if (param_2 >= param_3) {
        return;
    }
    int local_1 = param_1[param_3];
    int local_2 = param_2;
    for (int local_3 = param_2; local_3 < param_3; ++local_3) {
        if (param_1[local_3] <= local_1) {
            int local_4 = param_1[local_2];
            param_1[local_2] = param_1[local_3];
            param_1[local_3] = local_4;
            ++local_2;
        }
    }
    int local_5 = param_1[local_2];
    param_1[local_2] = param_1[param_3];
    param_1[param_3] = local_5;
    func_1(param_1, param_2, local_2 - 1);
    func_1(param_1, local_2 + 1, param_3);
}
