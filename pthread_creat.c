//创建

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

static void *func(void *p){
    puts("thread is working!");
    return NULL;
}

int main(){
    pthread_t   tid;
    int err;
    puts("begin!");
    err=pthread_create(&tid,NULL,func,NULL);//==================
    if(err){
        fprintf(stderr,"pthread_creat():%s\n ",strerror(err));

    }

    //分析：线程的调度取决于调度器策略。
    //       创建线程后，新的线程还没来得及被调度
    //，main线程就执行了exit(0)使得进程退出，所以新的线程并没有被执行就退出了。
    sleep(5);

    puts("end!");//init==1
     
    exit(0);
}