void func_1(int *param_1, int param_2) {
    for (int local_1 = param_2 / 2 - 1; local_1 >= 0; --local_1) {
        int local_2 = local_1;
        while (true) {
            int local_3 = 2 * local_2 + 1;
            if (local_3 >= param_2) {
                break;
            }
            if (local_3 + 1 < param_2 && param_1[local_3] < param_1[local_3 + 1]) {
                ++local_3;
            }
            if (param_1[local_2] >= param_1[local_3]) {
                break;
            }
            int local_4 = param_1[local_2];
            param_1[local_2] = param_1[local_3];
            param_1[local_3] = local_4;
            local_2 = local_3;
        }
    }
    for (int local_5 = param_2 - 1; local_5 > 0; --local_5) {
        int local_6 = param_1[0];
        param_1[0] = param_1[local_5];
        param_1[local_5] = local_6;
        int local_7 = 0;
        while (true) {
            int local_8 = 2 * local_7 + 1;
            if (local_8 >= local_5) {
                break;
            }
            if (local_8 + 1 < local_5 && param_1[local_8] < param_1[local_8 + 1]) {
                ++local_8;
            }
            if (param_1[local_7] >= param_1[local_8]) {
                break;
            }
            int local_9 = param_1[local_7];
            param_1[local_7] = param_1[local_8];
            param_1[local_8] = local_9;
            local_7 = local_8;
        }
    }
}
