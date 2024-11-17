#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

// 假设 strbuf 结构体已经被定义
struct strbuf {
    size_t len;
    size_t alloc;
    char *buf;
};

// 初始化 sb 结构体，容量为 alloc
void strbuf_init(struct strbuf *sb, size_t alloc) {
    if(sb=NULL)  return -1;
    sb->len = 0;
    sb->alloc = alloc;
    sb->buf = (char *)malloc(alloc * sizeof(char));
    if (sb->buf=NULL) {
        printf("malloc fail");
        return -1;
    }
    return 0;
}

// 将字符串填充到 sb 中，长度为 len，容量为 alloc
void strbuf_attach(struct strbuf *sb, void *str, size_t len, size_t alloc)
{
    if(str==NULL||sb==NULL)  return -1;
    size_t len=len+1;
    if(alloc < len)  return -1;
    if(sb->buf==NULL||sb->alloc<alloc){
        char *buf=realloc(sb->buf,alloc);
    }
    memcpy(sb->buf, str, len);
    sb->buf[len] = '\0'; // 确保字符串以空字符结尾
    sb->len = len; // 更新实际字符串长度
 
    return 0; 
}

// 释放 sb 结构体的内存
void strbuf_release(struct strbuf *sb)  
{
    free(sb->buf);
    sb->alloc=0;
    sb->len=0;
    sb->buf=NULL;
    return 0;
}  
        
// 交换两个 strbuf
void strbuf_swap(struct strbuf *a, struct strbuf *b)
{
    struct strbuf temp = *a;
    *a = *b;
    *b = temp;
    return 0;

}

// 将 sb 中的原始内存取出，并将 sz 设置为其 alloc 大小
char *strbuf_detach(struct strbuf *sb, size_t *sz)
{
    if(sb==NULL||sz==NULL)  return -1;
    *sz=sb->alloc;
    char *a=sb->buf;
    sb->alloc=0;
    sb->len=0;
    sb->buf=NULL;
    return *a;
}

// 比较两个 strbuf 的内存是否相同
int strbuf_cmp(const struct strbuf *first, const struct strbuf *second)
{
    if(first==NULL||second==NULL)  return -1;
    int min=(first->len<second->len)?first->len:second->len;
    int flag1=memcmp(first,second,min);
    int flag2;
    if(first->len==second->len)  flag2=1;
    if(flag1==1&&flag2==1) return 0;
    else return -1;
      
}

// 清空 sb
void strbuf_reset(struct strbuf *sb)
{
   if(sb==NULL)  return -1;
   free(sb->buf);
   sb->buf = NULL;
   sb->len = 0;
   sb->alloc = 0;
   return 0;
}
 

