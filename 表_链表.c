#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct Node* createHeaderNode();//创建头节点

void insertfirst(struct Node* header,int x);//从列表头部插入
void insertlast(struct Node* header,int x);//从列表尾部插入

struct Node* find(struct Node* header,int x);//查找元素
struct Node* findkth(struct Node* header,int position);//查找元素位置

void delete(struct Node* header,int x);//删除某元素

bool isEmpty(struct Node* header);

int size(struct Node* header);//统计节点个数
void printlist(struct Node* header);//打印列表

struct Node{
    int element;
    struct Node* next;
};

int main(){
    struct Node* node;//用来临时存储
    struct Node* header;

    //创建表头（哑节点）
    header=createHeaderNode();

    //判断表是否为空表
    printf("List is empty?  %d\n",isEmpty(header));
    printf("-------\n");

    //从列表头部插入
    insertfirst(header,1);
    insertfirst(header,2);

    printlist(header);
    printf("-------\n");

    //从列表尾部插入
    insertlast(header,3);
    insertlast(header,4);

    printlist(header);
    printf("-------\n");

    //判断表是否为空表
    printf("List is empty?  %d\n",isEmpty(header));
    printf("-------\n");

    //查找节点值为3的节点
    node=find(header,3);
    if(node==NULL)   printf("not find\n");
    else             printf("find node element : %d\n",node->element);
    printf("-------\n");

    //查找表中第二个节点
    node=findkth(header,2);
    if(node==NULL)   printf("not find\n");
    else             printf("find node element : %d\n",node->element);
    printf("-------\n");

    //删除结点值为3的结点
    delete(header,3);
    printlist(header);
    printf("-------\n");

    //打印表中结点个数，即表的大小
    printf("List size : %d\n",size(header));
    printf("-------\n");

    
}



//创建表头（哑节点）
struct Node* createHeaderNode(){
    struct Node* p;
    p=malloc(sizeof(struct Node));
    if(p==NULL){
        printf("内存分配失败\n");
        exit(1);
    }
    p->next=NULL;
    return p;
}

//判断表是否为空表
bool isEmpty(struct Node* header){
    return header->next==NULL;
}

//从列表头部插入
void insertfirst(struct Node* header,int x){
    struct Node* tmp;
    tmp=malloc(sizeof(struct Node));
    if(tmp==NULL){
        printf("内存分配失败\n");
        exit(1);
    }
    tmp->element=x;
    tmp->next=header->next;
    header->next=tmp;
    //free(tmp);
}

//从列表头部插入
void insertlast(struct Node* header,int x){
    struct Node* p;
    struct Node* tmp;
    tmp=malloc(sizeof(struct Node));
    if(tmp==NULL){
        printf("内存分配失败\n");
        exit(1);
    }
    tmp->element=x;
    tmp->next=NULL;

    p=header;

    while(p->next!=NULL){
        p=p->next;
    }

    p->next=tmp;
}

//打印列表
void printlist(struct Node* header){
    struct  Node* p;
    p=header->next;
    while(p!=NULL){
        printf("node element = %d\n",p->element);
        p=p->next;
    }

}


//查找元素,如果没有返回NULL
struct Node* find(struct Node* header,int x){
    struct Node* p;
    p=header->next;
    while(p!=NULL && p->element!=x)  p=p->next;
    return p;
}

//查找元素位置为position的节点，如果没有返回NULL
struct Node* findkth(struct Node* header,int position){
    int count=1;
    struct Node* p;
    if(position<=0){
        printf("position不能为负数\n");
        return NULL;
    }
    p=header->next;

    while (p!=NULL)
    {
        if(count==position)  return p;
        p=p->next;
        count++;
    }
    return NULL;
}


//删除第一个值匹配的点
void delete(struct Node* header,int x){
    struct  Node* privious;
    struct  Node* p;

    privious=header;
    p=header->next;

    while(p!=NULL){
        if(p->element==x){
            privious->next=p->next;
            free(p);
            break;
        }
        else{
            privious=p;
            p=p->next;
        }
    }  
}

//表中结点个数
int size(struct Node* header){
    int count=0;
    struct Node* p;
    p=header->next;
    while(p!=NULL){
        count++;
        p=p->next;
    }
    return count;
}





