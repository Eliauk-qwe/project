/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     struct ListNode *next;
 * };
 */
struct ListNode* rotateRight(struct ListNode* head, int k) {
    //对于长度为0和1的链表，怎么旋转都是本身，直接返回
    if(head == NULL || head->next == NULL)
    {
        return head;
    }

// 计算链表长度
    struct ListNode *temp = head;
    int length = 1;
    while (temp->next != NULL) {
        temp = temp->next;
        length++;
    }

    // 计算实际需要旋转的步数
    k = k % length;
    if (k == 0) {
        return head;
    }

    // 找到旋转点
    struct ListNode *rear= head;
    struct ListNode *front = head;
    for (int i = 0; i < k; i++) {
        front = front->next;
    }

    // 同步移动 rear 和 front，直到 front 指向链表的最后一个节点
    while (front->next != NULL) {
        front = front->next;
        rear = rear->next;
    }

    // 重新安排链表
    struct ListNode *newHead = rear->next;
    front->next = head;
    rear->next = NULL;

    return newHead;

}

