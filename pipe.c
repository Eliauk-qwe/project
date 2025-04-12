#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFSIZE        1024

void main(void) {
    int pd[2];
    char buf[BUFSIZE];
    pid_t pid;
    int len;

    // 创建匿名管道
    if(pipe(pd) < 0) {
        perror("pipe()");
        exit(1);
    }

    // 创建子进程
    pid = fork();
    if(pid < 0) {
        perror("fork()");
        exit(1);
    }
    if(pid == 0) { // 子进程 读取管道数据
        // 关闭写端
        close(pd[1]);
        // 从管道中读取数据，如果子进程比父进程先被调度会阻塞等待数据写入
        len = read(pd[0], buf, BUFSIZE);
        puts(buf);
        // 管道是fork之前创建的，父子进程里都有一份，所以退出之前要确保管道两端都关闭
        close(pd[0]);
        exit(0);
    } else { // 父进程 向管道写入数据
        // 关闭读端
        close(pd[0]);
        write(pd[1], "Hello!", 6);
        close(pd[1]);
        // 等待子进程退出
        wait(NULL);
        exit(0);
    }
    // never reached
    exit(0);
}
