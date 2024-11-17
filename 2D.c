#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



// 假设 strbuf 结构体已经被定义
struct strbuf {
    size_t len;
    size_t alloc;
    char *buf;
};

// 确保在 len 之后 strbuf 中至少有 extra 个字节的空闲空间可用。
void strbuf_grow(struct strbuf *sb, size_t extra)
{
    size_t alloc =sb->alloc;
    while(sb->alloc -sb->len <extra)
      alloc++;
    char *buf=(char*)realloc(sb->buf,alloc);
    sb->alloc=alloc;
    sb->buf=buf;
}


// 将文件描述符为 fd 的所有文件内容追加到 sb 中。sb 增长 hint ? hint : 8192 大小。
ssize_t strbuf_read(struct strbuf *sb, int fd, size_t hint)
{
    size_t need_read=hint?hint:8192;
    int per_read;
    size_t total_read=0;
    // 若 sb 未初始化，则进行初始化
    if(!sb->buf)
    {
        sb->buf=malloc(need_read);
        if(!sb->buf)  return -1;
    }

    if(sb->alloc<need_read)  strbuf_grow(sb,need_read-sb->alloc);
    sb->alloc=need_read;
    sb->len=0;

    while ((per_read=read(fd,sb->buf,sb->alloc-sb->len))>0)
    {
        total_read+=per_read;
        sb->len+=per_read;
    }

    if(per_read==-1)  return -1;
    return total_read;
    


}