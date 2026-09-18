int func_1(const void *param_1, const void *param_2, int param_3) {
    const unsigned char *local_1 = static_cast<const unsigned char *>(param_1);
    const unsigned char *local_2 = static_cast<const unsigned char *>(param_2);
    for (int local_3 = 0; local_3 < param_3; ++local_3) {
        if (local_1[local_3] != local_2[local_3]) {
            return local_1[local_3] < local_2[local_3] ? -1 : 1;
        }
    }
    return 0;
}
