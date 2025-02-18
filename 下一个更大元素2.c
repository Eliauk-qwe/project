/**
 * Note: The returned array must be malloced, assume caller calls free().
 */
int* nextGreaterElement(int* nums1, int nums1Size, int* nums2, int nums2Size, int* returnSize) {
    *returnSize = nums1Size;            // returnSize和nums1的值相同
    int* ans = (int*)malloc(sizeof(int) * nums1Size);
    memset(ans, 0, sizeof(int) * nums1Size);

    int table[10001] = {0};                    
    int index = 0;

    // table的索引是nums2的元素，table的元素是nums2中，每个元素在数组中的位置
    for(int i = 0; i < nums2Size; i++){
        table[nums2[i]] = index++;
    }
    // 遍历nums1中的元素
    for(int i = 0; i < nums1Size; i++){
        // 从table中直接找到nums1中元素的位置，然后从nums2的此位置向后找更大元素
        for(int j = table[nums1[i]] + 1; j < nums2Size; j++){
            if(nums2[j] > nums1[i]) {
                ans[i] = nums2[j];
                break;
            }
        }
        // 若条件满足，说明没有找到更大值，赋值为-1
        if(ans[i] == 0) ans[i] = -1;
    }
    return ans;
}

