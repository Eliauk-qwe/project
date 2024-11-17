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

// 向 sb 追加长度为 len 的数据 data。
void strbuf_add(struct strbuf *sb, const void *data, size_t len)
{
    if(sb->len+len>sb->alloc)
    {
        strbuf_grow(sb,len);
    }

    memcpy(sb,data,len);
    sb->len=sb->len+len;
}

/*向 sb 追加一个字符 c。
void strbuf_addch(struct strbuf *sb, int c)
{
    char ch=(char)c;
    size_t len=strlen(ch);
    void strbuf_add(sb,&ch,len);
}*/

// 向 sb 追加一个字符串 s。
void strbuf_addstr(struct strbuf *sb, const char *s)
{
    size_t len=strlen(s);
     strbuf_add(sb,s,len);
}

// 向一个 sb 追加另一个 strbuf 的数据。
void strbuf_addbuf(struct strbuf *sb, const struct strbuf *sb2)
{
     strbuf_add(sb,sb2->buf,sb2->len);

}

// 设置 sb 的长度 len。
void strbuf_setlen(struct strbuf *sb, size_t len)
{
    if(sb->alloc<len)
    strbuf_grow(sb, len-sb->alloc);
    sb->len=len;

}

// 计算 sb 目前仍可以向后追加的字符串长度。
size_t strbuf_avail(const struct strbuf *sb)
{
    return sb->alloc-sb->len;
}

// 向 sb 内存坐标为 pos 位置插入长度为 len 的数据 data。
void strbuf_insert(struct strbuf *sb, size_t pos, const void *data, size_t len)
{
    if(pos>sb->len) pos=sb->len;
    if(sb->len+len>sb->alloc)     strbuf_add(sb, data, sb->len+len-sb->alloc);\
    memmove(sb->buf+pos+len,sb->buf+pos,sb->len-pos);
    memcpy(sb->buf+pos,data,len);
    sb->len+=len;
}




