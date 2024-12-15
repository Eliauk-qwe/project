char * baseNeg2(int n) {
    if (n == 0) {
        return "0";
    }
    if (n == 1) {
        return "1";
    }
    char *res = (char *)calloc(sizeof(char), 32);
    int pos = 0;
    while (n != 0) {
        int remainder = n & 1;
        res[pos++] = '0' + remainder;
        n -= remainder;
        n /= -2;
    }
    for (int l = 0, r = pos - 1; l < r; l++, r--) {
        char c = res[l];
        res[l] = res[r];
        res[r] = c;
    }
    return res;
}

