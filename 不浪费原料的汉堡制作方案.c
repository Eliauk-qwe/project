int* numOfBurgers(int tomatoSlices, int cheeseSlices, int* returnSize) {
if (tomatoSlices % 2 != 0 || tomatoSlices < cheeseSlices * 2 || cheeseSlices * 4 < tomatoSlices) {
        *returnSize = 0;
        return NULL;
    }
    int *ans = (int *)malloc(sizeof(int) * 2);
    ans[0] = tomatoSlices / 2 - cheeseSlices;
    ans[1] = cheeseSlices * 2 - tomatoSlices / 2;
    *returnSize = 2;
    return ans;
}

