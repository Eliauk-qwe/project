//栈的清理     !!!
//  pthread_cleanup_push   (
//  pthread_cleanup_pop    )

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static void cleanup_func(void *p){
    puts(p);
}


static void *func(void *p){
    puts("thread is working!");

    //栈的清理  ======================
    pthread_cleanup_push(cleanup_func,"cleanup:1");
    pthread_cleanup_push(cleanup_func,"cleanup:2");
    pthread_cleanup_push(cleanup_func,"cleanup:3");

    puts("over");

    
//一定有，执行不到，默认1   ====================
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(1);
    
    pthread_exit(NULL);
    
}

int main(){
    pthread_t   tid;
    int err;
    puts("begin!");
    err=pthread_create(&tid,NULL,func,NULL);
    if(err){
        fprintf(stderr,"pthread_creat():%s\n ",strerror(err));
    }
    pthread_join(tid,NULL);
    puts("end!");
    exit(0);
}
