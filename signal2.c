#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
static  void int_handler(int s){
        write(1,"!",1);

}
int main(){
    
    //程序没结束，信号来了执行
    signal(SIGINT,int_handler);

    //信号会打断阻塞的系统调用
    //一直ctrl +c

    for(int i=0;i<10;i++){
        write(1,"*",1);
        sleep(1);
    }
    printf("\n");

    exit(0);
}

/*==================================

do {
    sfd = open(argv[1], O_RDONLY);  // 尝试打开源文件
    if (sfd < 0) {                  // 打开失败
        if (errno != EINTR) {       // 如果错误不是信号中断（EINTR）
            perror("open()");       // 打印错误信息
            exit(1);                // 退出程序
        }
        // 如果是 EINTR（信号中断），则继续循环，重试 open()
    }
} while (sfd < 0);                 // 循环直到成功打开文件

=============================================*/