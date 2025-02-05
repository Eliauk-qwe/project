// 比较函数，用于qsort排序
int compare(const void *a, const void *b) {
    return *(int *)a > *(int *)b ? 1:-1;
}

int smallestDifference(int *a, int aSize, int *b, int bSize) {
    // 对数组a和b进行排序
    qsort(a, aSize, sizeof(int), compare);
    qsort(b, bSize, sizeof(int), compare);

    int i = 0, j = 0;
    long long  minDiff = LLONG_MAX;

    while (i < aSize && j < bSize) {
        // 计算当前两个元素差值的绝对值，并更新最小差值
        long long diff = labs((long long)a[i]-(long long)b[j]);
        minDiff = (long long)fmin(minDiff, diff);
        if (a[i] < b[j]) {
            i++;
        } else {
            j++;
        }
    }
    return (int)minDiff;
}

