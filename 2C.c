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

// 去除 sb 缓冲区左端的所有空格、制表符和'\t'字符。
void strbuf_ltrim(struct strbuf *sb)
{
    const char *need_del=" \t";
    int start=0;
    while (start<sb->len&&strchr(need_del,sb->buf[start]))
    {
        start++;
    }
    
    if (start>0)
    {
        memmove(sb->buf,sb->buf+start,sb->len-start);
    }
    sb->len=sb->len-start;
    
}

// 去除 sb 缓冲区右端的所有空格、制表符和'\t'字符。
void strbuf_rtrim(struct strbuf *sb)
{
    const char *need_del=" \t";
    int end=sb->len-1;
    while (end>=0&&strchr(need_del,sb->buf[end]))
    {
        end--;
    }
    if(end<sb->len-1)
    {
        sb->buf[end+1]='\0';
        sb->len=end+1;
    }
    
}

// 删除 sb 缓冲区从 pos 坐标开始长度为 len 的内容。
void strbuf_remove(struct strbuf *sb, size_t pos, size_t len)
{
    if (pos < 0 || pos > sb->len || pos + len > sb->len ) {
        
        exit(EXIT_FAILURE);
    }
    memmove(sb->buf+pos,sb->buf+pos+len,sb->len-len-pos);
    sb->len=sb->len-len;

}
