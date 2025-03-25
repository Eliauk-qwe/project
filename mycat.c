#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>  // 添加缺失的头文件

#define BUFSIZE 1024

int main(int argc, char **argv) {
    int sfd, dfd=1;
    char buf[BUFSIZE];
    int len, ret, pos;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <> \n", argv[0]);
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
    } while (sfd < 0);  // 补充分号

    

    // 读取并写入数据
    while (1) {
        len = read(sfd, buf, BUFSIZE);
        if (len < 0) {
            if (errno == EINTR) continue;
            perror("read()");
            close(dfd);  // 关闭文件描述符
            close(sfd);
            exit(1);    // 返回错误状态
        }
        if (len == 0) break;  // 正常读取完毕

        pos = 0;
        while (len > 0) {
            ret = write(dfd, buf + pos, len);
            if (ret < 0) {
                if (errno == EINTR) continue;
                perror("write()");
                close(dfd);
                close(sfd);
                exit(1);
            }
            pos += ret;
            len -= ret;
        }
    }

    
    close(sfd);
    exit(0);
}