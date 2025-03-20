//exmple1

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>  // 添加缺失的头文件
#include <signal.h>
#include <sys/time.h>

#define BUFSIZE CPS
#define CPS  10

static volatile int flag=0;

static void alrm_hander(int s){
    //alarm(1);
    flag=1;
}

int main(int argc, char **argv) {
    int sfd, dfd=1;
    char buf[BUFSIZE];
    int len, ret, pos;
    struct itimerval itv;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <> \n", argv[0]);
        exit(1);
    }
  
  
    signal(SIGALRM,alrm_hander);
    
    itv.it_interval.tv_sec=1;
    itv.it_interval.tv_usec=0;
    itv.it_value.tv_sec=1;
    itv.it_value.tv_usec=0;

    if(setitimer(ITIMER_REAL,&itv,NULL) <0){
        perror("setitimer()");
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
        while(!flag)  pause();

        flag=0;
        while((len = read(sfd, buf, BUFSIZE))<0){
        if (len < 0) {
            if (errno == EINTR) continue;
            perror("read()");
            break;
        }
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
        //sleep(1);
    }

    
    close(sfd);
    exit(0);
}

//exmple2
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>

void handler(int sig) {
    printf("Timer expired!\n");
}

int main() {
    struct itimerval timer;
    
    // 首次触发时间：1秒后
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    
    // 重复间隔：2秒
    timer.it_interval.tv_sec = 2;
    timer.it_interval.tv_usec = 0;

    // 注册信号处理函数
    signal(SIGALRM, handler);

    // 设置实时定时器
    setitimer(ITIMER_REAL, &timer, NULL);

    // 保持进程运行
    while(1);
    return 0;
}