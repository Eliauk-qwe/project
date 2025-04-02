//线程的取消选项   ！!!若取消一个正在进行的线程，先cancel，再join

// 发送终止信号给thread线程，如果成功则返回0，否则为非0值。发送成功并不意味着thread会终止。
//int pthread_cancel(pthread_t thread);
/*  取消有两种状态
    1.允许取消：  异步cancel
                推迟cancel  ->推迟至cancel点响应
                     cancel点：POSIX定义的cancel点，都是可能引发阻塞的系统调用
    2.不允许
*/

// 设置本线程取消动作的执行时机
//int pthread_setcanceltype(int type, int *oldtype)  

// 在不包含取消点，但是又需要取消点的地方创建一个取消点，以便在一个没有包含取消点的执行代码线程中响应取消请求
//nothing，就是一个取消点
//void pthread_testcancel(void)

/*#include <pthread.h>

       int pthread_setcancelstate(int state, int *oldstate);  //设置是否允许取消
       int pthread_setcanceltype(int type, int *oldtype);     //设置取消方式
*/








//线程的分离     线程的分离不能被join
/*include <pthread.h>

       int pthread_detach(pthread_t thread);
*/







#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

//线程执行的函数
void * thread_Fun(void * arg) {
    printf("新建线程开始执行\n");
    sleep(10);
}

int main(void) {
    pthread_t myThread;
    void * mess;
    int value;
    int res;
    //创建 myThread 线程
    res = pthread_create(&myThread, NULL, thread_Fun, NULL);
    if (res != 0) {
        printf("线程创建失败\n");
        return 0;
    }
    sleep(1);
    //向 myThread 线程发送 Cancel 信号
    res = pthread_cancel(myThread);
    if (res != 0) {
        printf("终止 myThread 线程失败\n");
        return 0;
    }
    //获取已终止线程的返回值
    res = pthread_join(myThread, &mess);
    if (res != 0) {
        printf("等待线程失败\n");
        return 0;
    }
    //如果线程被强制终止，其返回值为 PTHREAD_CANCELED
    if (mess == PTHREAD_CANCELED) {
        printf("myThread 线程被强制终止\n");
    }
    else {
        printf("error\n");
    }
    return 0;
}


