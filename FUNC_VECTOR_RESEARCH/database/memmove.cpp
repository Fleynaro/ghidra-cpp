void *func_1(void *param_1, const void *param_2, int param_3) {
    unsigned char *local_1 = static_cast<unsigned char *>(param_1);
    const unsigned char *local_2 = static_cast<const unsigned char *>(param_2);
    if (local_1 < local_2) {
        for (int local_3 = 0; local_3 < param_3; ++local_3) {
            local_1[local_3] = local_2[local_3];
        }
    } else if (local_1 > local_2) {
        for (int local_3 = param_3; local_3 > 0; --local_3) {
            local_1[local_3 - 1] = local_2[local_3 - 1];
        }
    }
    return param_1;
}
