void *func_1(void *param_1, const void *param_2, int param_3) {
    unsigned char *local_1 = static_cast<unsigned char *>(param_1);
    const unsigned char *local_2 = static_cast<const unsigned char *>(param_2);
    if (local_1 == local_2 || param_3 == 0) {
        return param_1;
    }
    if (local_1 < local_2) {
        int local_3 = 0;
        while (local_3 < param_3) {
            local_1[local_3] = local_2[local_3];
            ++local_3;
        }
    } else {
        int local_3 = param_3;
        while (local_3 != 0) {
            --local_3;
            local_1[local_3] = local_2[local_3];
        }
    }
    return param_1;
}
