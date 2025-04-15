#ifndef PROTO_H__
#define PROTO_H__

#define NAMESIZE  11  //对齐

#define REVPORT   "1989"  //1980+

#define MTGOURP "224.2.2.2" // 约定的多播组ip

#include <stdint.h>   //头文件！！
struct msg_st{
    uint8_t name[NAMESIZE];
    uint32_t math;
    uint32_t chinese;
}__attribute__((packed));











#endif