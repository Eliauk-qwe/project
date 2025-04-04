int* nextGreaterElements(int* nums, int numsSize, int* returnSize) {
    *returnSize = numsSize;
    if (numsSize == 0) {
        return NULL;
    }
    int* ret = malloc(sizeof(int) * numsSize);
    memset(ret, -1, sizeof(int) * numsSize);

    int stk[numsSize * 2 - 1], top = 0;
    for (int i = 0; i < numsSize * 2 - 1; i++) {
        while (top > 0 && nums[stk[top - 1]] < nums[i % numsSize]) {
            ret[stk[top - 1]] = nums[i % numsSize];
            top--;
        }
        stk[top++] = i % numsSize;
    }
    return ret;
}


