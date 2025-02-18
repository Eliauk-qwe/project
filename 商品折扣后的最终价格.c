/**
 * Note: The returned array must be malloced, assume caller calls free().
 */
int* finalPrices(int* prices, int pricesSize, int* returnSize){
    *returnSize = pricesSize;
    int* ret = (int*) malloc( sizeof(int) * pricesSize );
    //  单调递减栈
    int* stack = (int*) malloc( sizeof(int) * pricesSize );
    int top = -1;

    for(int i = 0; i < pricesSize; i++){
        //  先记录原价
        ret[i] = prices[i];
        while( top >= 0 && prices[i] <= prices[ stack[top] ] ){
            //  每个商品都记录了原价，直接减去折扣
            ret[ stack[top] ] -= prices[i]; 
            top--;
        }
        stack[ ++top ] = i;
    }

    return ret;
}

