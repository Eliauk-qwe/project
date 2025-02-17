/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     struct ListNode *next;
 * };
 */
struct ListNode* addTwoNumbers(struct ListNode* l1, struct ListNode* l2) {
    struct ListNode* result=(struct ListNode*)malloc(sizeof(struct ListNode));
    result->val=0;
    result->next=NULL;//初始化结果链表
    struct ListNode* h=result;//设置链表指针
    if(l1->val==0 && l2->val==0 && l1->next==NULL && l2->next==NULL)return result;//判断是否0+0
    else{
        int high=0;//进位提示
        int sum=0;//每一位的和
        int n1=0;//每一位的数字
        int n2=0;//每一位的数字，考虑到链表长度不一致，需要设置为0
        while(l1!=NULL||l2!=NULL){
            if(l1!=NULL){
                n1=l1->val;
                l1=l1->next;
            }else{
                n1=0;
            }
            if(l2!=NULL){
                n2=l2->val;
                l2=l2->next;
            }else{
                n2=0;
            }//取出当前位的数字，null则设置为0，并将链表指向下一位
            sum=n1+n2+high;
            if(sum>=10)high=1;
            else high=0;//求和并考虑是否存在进位
            struct ListNode* newNode=(struct ListNode*)malloc(sizeof(struct ListNode));
            newNode->val = sum%10;
            newNode->next=NULL;//构建新链表，将该位数字储存
            h->next=newNode;//将新链表链接在输出结果后面
            h=newNode;//链表指针后移一位
        }
        if(high==1){
            struct ListNode* newNode=(struct ListNode*)malloc(sizeof(struct ListNode));
            newNode->val = 1;
            newNode->next=NULL;
            h->next=newNode;
            h=newNode;
        }//在will结束后，可能仍有进位，如果有则将进位1添加在最后
        return result->next;//返回除了第一位0以外的结果
    }
    return 0;
}

