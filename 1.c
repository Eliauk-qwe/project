/**
 * Return an array of arrays of size *returnSize.
 * The sizes of the arrays are returned as *returnColumnSizes array.
 * Note: Both returned array and *columnSizes array must be malloced, assume caller calls free().
 */
int cmp(const void *a, const void *b) {
    return *(int*)a - *(int*)b;
}

void dfs(int i, int n, int* nums, int** ans, int* returnSize, int** columnSizes, int* path, int pathSize) {
    if (i == n) {
        ans[*returnSize] = malloc(sizeof(int) * pathSize);
        memcpy(ans[*returnSize], path, sizeof(int) * pathSize);
        (*columnSizes)[(*returnSize)++] = pathSize;
        return;
    }

    // 选 x
    int x = nums[i];
    path[pathSize] = x;
    dfs(i + 1, n, nums, ans, returnSize, columnSizes, path, pathSize + 1);

    // 不选 x，那么后面所有等于 x 的数都不选
    // 如果不跳过这些数，会导致「选 x 不选 x'」和「不选 x 选 x'」这两种情况都会加到 ans 中，这就重复了
    i++;
    while (i < n && nums[i] == x) {
        i++;
    }
    dfs(i, n, nums, ans, returnSize, columnSizes, path, pathSize);
}

int** subsetsWithDup(int* nums, int n, int* returnSize, int** columnSizes) {
    qsort(nums, n, sizeof(int), cmp);

    int m = 1 << n; // 至多有 2^n 个子集
    int** ans = malloc(sizeof(int*) * m);
    *returnSize = 0;
    *columnSizes = malloc(sizeof(int) * m);

    int* path = malloc(sizeof(int) * n);
    dfs(0, n, nums, ans, returnSize, columnSizes, path, 0);

    free(path);
    return ans;
}

