#include <stdio.h>
#include <stdlib.h>

#ifndef _LIST_H

// 链表类型声明
struct Node;
typedef struct Node* PtrToNode;
typedef PtrToNode List;
typedef PtrToNode Position;
typedef double ElementType;

// 函数声明
List MakeEmpty(List L);
int IsEmpty(List L);
int IsLast(Position P, List L);
Position Find(ElementType X, List L);
void Delete(ElementType X, List L);
Position FindPrevious(ElementType X, List L);
void Insert(ElementType X, List L, Position P);
void DeleteList(List L);
Position Header(List L);
Position First(List L);
Position Advance(Position P);
ElementType Retrieve(Position P);

#endif

// 链表节点完整定义
struct Node {
    ElementType Element;
    Position Next;
};

// 链表操作函数实现
int IsEmpty(List L) {
    return L->Next == NULL;
}

int IsLast(Position P, List L) {
    return P->Next == NULL;
}

Position Find(ElementType X, List L) {
    Position P = L->Next;
    while (P != NULL && P->Element != X)
        P = P->Next;
    return P;
}

Position FindPrevious(ElementType X, List L) {
    Position P = L;
    while (P->Next != NULL && P->Next->Element != X)
        P = P->Next;
    return P;
}

void Delete(ElementType X, List L) {
    Position P = FindPrevious(X, L);
    if (!IsLast(P, L)) {
        Position TmpCell = P->Next;
        P->Next = TmpCell->Next;
        free(TmpCell);
    }
}

void Insert(ElementType X, List L, Position P) {
    Position TmpCell = malloc(sizeof(struct Node));
    if (TmpCell == NULL) {
        printf("OUT OF SPACE!!!");
        return;
    }
    TmpCell->Element = X;
    TmpCell->Next = P->Next;
    P->Next = TmpCell;
}

// 补全的函数实现
List MakeEmpty(List L) {
    if (L == NULL) {
        L = (List)malloc(sizeof(struct Node));
        if (L == NULL) {
            printf("Out of memory!\n");
            return NULL;
        }
        L->Next = NULL;
    } else {
        Position P = L->Next;
        while (P != NULL) {
            Position tmp = P;
            P = P->Next;
            free(tmp);
        }
        L->Next = NULL;
    }
    return L;
}

void DeleteList(List L) {
    MakeEmpty(L);
    free(L);
}

Position Header(List L) {
    return L;
}

Position First(List L) {
    return L->Next;
}

Position Advance(Position P) {
    return P->Next;
}

ElementType Retrieve(Position P) {
    return P->Element;
}

// 测试主函数
int main() {
    // 测试MakeEmpty和IsEmpty
    List myList = MakeEmpty(NULL);
    printf("初始化后链表是否为空？%s\n", IsEmpty(myList) ? "是" : "否");  // 应输出"是"

    // 测试Insert/Header/First/Advance/Retrieve
    Insert(1.1, myList, Header(myList));  // 插入到头部
    Insert(2.2, myList, Header(myList));
    Insert(3.3, myList, Header(myList));
    
    // 遍历测试
    printf("插入元素后链表内容：");
    Position current = First(myList);
    while(current != NULL) {
        printf("%.1f ", Retrieve(current));
        //current = Advance(current);
        current=current->Next;
    }
    printf("\n");  // 应输出 3.3 2.2 1.1

    // 测试Find和FindPrevious
    Position found = Find(2.2, myList);
    if(found) printf("找到元素：%.1f\n", Retrieve(found));
    
    Position prev = FindPrevious(1.1, myList);
    if(prev) printf("1.1的前驱节点是：%.1f\n", Retrieve(prev));  // 应显示2.2

    // 测试IsLast
    Position lastNode = First(myList);
    while(lastNode->Next != NULL) lastNode = Advance(lastNode);
    printf("最后一个节点是末尾吗？%s\n", IsLast(lastNode, myList) ? "是" : "否");

    // 测试Delete
    Delete(2.2, myList);
    printf("删除2.2后内容：");
    for(Position p = First(myList); p; p = Advance(p)) {
        printf("%.1f ", Retrieve(p));
    }
    printf("\n");  // 应输出3.3 1.1

    // 测试清空链表
    printf("清空前链表长度：");
    int count = 0;
    for(Position p = First(myList); p; p = Advance(p)) count++;
    printf("%d\n", count);  // 应显示2
    
    myList = MakeEmpty(myList);  // 清空现有链表
    printf("清空后是否为空？%s\n", IsEmpty(myList) ? "是" : "否");

    // 测试边界条件
    Position invalid = Find(99.9, myList);
    if(!invalid) printf("未找到不存在的元素\n");

    // 最终清理
    DeleteList(myList);
    return 0;
}