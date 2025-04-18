struct ListNode* removeNthFromEnd(struct ListNode* head, int n) {
    struct ListNode **p = &head, *q = head;
    for (int i = 0; i < n; ++i)
        q = q->next;

    while (q) {
        p = &((*p)->next);
        q = q->next;
    }
    *p = (*p)->next;
    return head;
}

