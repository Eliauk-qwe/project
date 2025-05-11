#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "proto.h"


int main() {
    key_t key;
    int msgid;
    struct msg_st sbuf;

    // 设置随机数种子
    srand(time(NULL));
    // 用与接收方相同的文件和 salt 生成一样的key，这样才可以通讯
    key = ftok(KEYPATH, KEYPROJ);
    if(key < 0) {
        perror("ftok()");
        exit(1);
    }

    // 取得消息队列
    msgid = msgget(key, 0);
    if(msgid < 0) {
        perror("msgget()");
        exit(1);
    }

    // 为要发送的结构体赋值
    sbuf.mtype = MSGTYPE;
    strcpy(sbuf.name, "Alan");
    sbuf.math = rand() % 100;
    sbuf.chinese = rand() % 100;

    // 发送结构体
    if(msgsnd(msgid, &sbuf, sizeof(sbuf)-sizeof(long), 0) < 0) {
        perror("msgsnd()");
        exit(1);
    }
    puts("ok!");
    // 消息队列不是发送方创建的，所以发送方不用负责销毁
    exit(0);
}
