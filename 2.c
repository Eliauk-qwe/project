
int findString(char** words, int wordsSize, char* s){
    int left = 0, right = wordsSize-1, mid;
    while(left<right){
        mid = (left + right) / 2;
        if (*words[mid] == NULL) {//如果中间为空，则由二分查找变为线性遍历
            if(strcmp(words[left],s)) left++;//从左至右扫描
            else return left; 
        }
        else if (strcmp(words[mid],s) > 0) right = mid - 1;
        else if (strcmp(words[mid],s) < 0) left = mid + 1;
        else return mid;
    }
    if (strcmp(words[left],s) == 0) return left;
    else return -1;
}

作者：星野爱
链接：https://leetcode.cn/problems/sparse-array-search-lcci/solutions/2332805/jian-dan-de-er-fen-cha-zhao-xian-ding-qi-ste2/
来源：力扣（LeetCode）
著作权归作者所有。商业转载请联系作者获得授权，非商业转载请注明出处。