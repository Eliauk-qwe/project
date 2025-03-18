#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include "mytbf.h"  // 头文件名修正
#include <string.h>

#define BUFSIZE 1024
#define CPS 10
#define BURST 100

int main(int argc, char **argv) {
    int sfd, dfd = 1;
    char buf[BUFSIZE];
    int len, ret, pos;
    mytbf_t *tbf;
    int size;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source>\n", argv[0]);
        exit(1);
    }

    tbf = mytbf_init(CPS, BURST);
    if (tbf == NULL) {
        fprintf(stderr, "mytbf_init() failed!\n");
        exit(1);
    }

    // 打开源文件（处理信号中断）
    do {
        sfd = open(argv[1], O_RDONLY);
        if (sfd < 0) {
            if (errno != EINTR) {
                perror("open()");
                exit(1);
            }
        }
    } while (sfd < 0);

    // 读取并写入数据
    while (1) {
        size = mytbf_fetchtoken(tbf, BUFSIZE);
        if (size < 0) {
            fprintf(stderr, "mytbf_fetchtoken(): %s\n", strerror(-size));
            exit(1);
        }

        // 修正读取逻辑：直接处理 read 返回值
        len = read(sfd, buf, size);
        if (len < 0) {
            if (errno == EINTR) continue;
            perror("read()");
            break;
        }
        if (len == 0) break;

        // 归还未使用的令牌
        if (size - len > 0) {
            mytbf_returntoken(tbf, size - len);
        }

        pos = 0;
        while (len > 0) {
            ret = write(dfd, buf + pos, len);
            if (ret < 0) {
                if (errno == EINTR) continue;
                perror("write()");
                close(sfd);
                exit(1);
            }
            pos += ret;
            len -= ret;
        }
    }

    close(sfd);
    mytbf_destroy(tbf);  // 修正函数名
    exit(0);
}