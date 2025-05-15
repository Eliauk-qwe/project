#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>

threadpool_t *pool=NULL;

#define CONTROL_PORT  2100
#define LISTEN_NUM    10
#define PTHREADPOOL_SIZE 10
#define QUEUE_SIZE      64
#define BUFSIZE       1024
#define MAX_CMD_LEN 512
#define MAX_USERNAME_LEN 32
#define MAX_PASS_LEN 32


//线程池结构体
typedef struct {
    pthread_mutex_t lock;          // 互斥锁
    pthread_cond_t cond;         // 条件变量（任务通知）
    pthread_t *threads;            // 线程数组指针
    int thread_count;              // 线程数量
    int queue_size;                // 任务队列容量
    int head, tail, count;         // 队列头尾指针及元素计数
    int shutdown;                  // 关闭标志
    void (*task_func)(void *);     // 任务处理函数指针
    void **task_args;              // 任务参数队列
} threadpool_t;

//任务参数结构
typedef struct{
    int control_fd;
    struct sockaddr_in client_addr;
}client_task_t;

// 简易用户验证 (用户名密码明文，实际使用需安全措施)
typedef struct {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASS_LEN];
} User;

User users[] = {
    {"test", "123456"},
    {"user", "pass"},
    {0}
};







//创建监听套接字函数
static int creat_listen_socket(int port);
//创建线程池
threadpool_t* creat_pthreadpool(int pth_num,int queue_num,void (*task_func) (void*));
//线程池中的线程
static void* pthreadpool_pthread(void* threadpool);
//服务端控制线程
void control_func(void *arg);
// 向客户端发消息，带格式化
static void ftp_send(int fd,const char* fmt,...);
//接受客户端的消息
static int ftp_recv(int fd,char *buf,int maxlen);
//比较用户名和密码
int diff_user_pass(const char *user,const char *pass);

int main(){

    //用于生成随机端口
    srand(time(NULL));

    int server_fd=creat_listen_socket(CONTROL_PORT);
    if(server_fd<0){
        fprintf(stderr,"creat_listen_socket() failed!\n");
        close(server_fd);
        return -1;
    }

    pool=creat_pthreadpool(PTHREADPOOL_SIZE,QUEUE_SIZE,control_func);






}


static int creat_listen_socket(int port){
    int fd;
    fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd<0){
        perror("socket()");
        return 1;
    }


    //允许地址重用
    int opt=1;
    setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    struct  sockaddr_in addr;

    memset(&addr,0,sizeof(addr));
    //无论主机字节序（小端或大端）如何，0 的二进制表示始终是全零，因此：不用htonl
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);

    socklen_t len =sizeof(addr);
    if(bind(fd,(struct sockaddr_in*)&addr,len) <0){
        perror("bind()");
        close(fd);
        return -1;
    }

    if(listen(fd,LISTEN_NUM) <0){
        perror("listen()");
        close(fd);
        return -1;
    }

    return fd;
}

threadpool_t* creat_pthreadpool(int pth_num,int queue_num,void (*task_func) (void*)){
    threadpool_t *pool =malloc(sizeof(threadpool_t));
    if(!pool){

    }

    pool->thread_count=pth_num;
    pool->queue_size=queue_num;
    pool->task_func=task_func;
    pool->head=pool->tail=pool->count=0;
    pool->shutdown=0;

    pthread_mutex_init(&pool->lock,NULL);
    pthread_cond_init(&pool->cond,NULL);

    pool->threads=malloc(sizeof(pthread_t)* pth_num);
    pool->task_args=malloc(sizeof(void*)*queue_num);

    for(int i=0 ;i< pth_num ;i++){
        pthread_create(&pool->threads[i],NULL,pthreadpool_pthread,pool);
    }

    return pool;

       
}

static void* pthreadpool_pthread(void* threadpool){
    threadpool_t *pool =(threadpool_t*)threadpool;
    while(1){
        pthread_mutex_lock(&pool->lock);
        // 当任务队列为空且线程池未关闭时，线程进入等待状态
        while (pool->count==0 && !pool->shutdown)
        {
            pthread_cond_wait(&pool->cond,&pool->lock);
        }
        // 如果线程池已关闭，解锁并退出线程
        if(pool->shutdown){
            pthread_mutex_unlock(&pool->lock);
            pthread_exit(NULL);
        }
        // 从任务队列中取出一个任务
        void *task =pool->task_args[pool->head];
        pool->head=(pool->head+1)%pool->queue_size;
        pool->count--;

        pthread_mutex_unlock(&pool->lock);
        // 执行任务
        pool->task_func(task);

    }

    return NULL;
}


void control_func(void *arg){
    client_task_t *task =(client_task_t*)arg;
    int ctrl_fd = task->control_fd;
    free(task);

    ftp_send(ctrl_fd,"Welcome to my FTP Server");

    char buf[MAX_CMD_LEN];
    char user[MAX_USERNAME_LEN] = {0};
    char pass[MAX_PASS_LEN] = {0};
    int flag=0;
    


    while(1){
        int recv=ftp_recv(ctrl_fd,buf,BUFSIZE);
        if(recv<0)  break;

        printf("CMD:%s",buf);

        if(strncmp(buf,"USER",4)==0){
            sscanf(buf+5,"%s",user);
            ftp_send(ctrl_fd, "331 Username OK, need password");
            
        }else if(strncmp(buf,"PASS",4)==0){
            sscanf(buf+5,"%s",pass);
            if(diff_user_pass(user,pass)){
                flag=1;
                ftp_send(ctrl_fd, "230 Login successful");
            }else{
                ftp_send(ctrl_fd, "530 Login incorrect");
                break;
            }

        }else if(!flag){
            ftp_send(ctrl_fd, "530 Please login with USER and PASS");
        }






    }

    
}


static void ftp_send(int fd, const char *fmt,...){
    char buf[BUFSIZE];

    va_list arg;

    va_start(arg,fmt);

    vsnprintf(buf,sizeof(buf),fmt,arg);

    va_end(arg);

    strcat(buf,"\r\n");

    send(fd,buf,sizeof(buf),0);
    
}

static int ftp_recv(int fd,char *buf,int maxlen){
    //已读取到字符数量
    int i=0;
    //读取到的字符
    int c='\0';
    int n;

    while(i< maxlen-1 && c != '\n'){
        n=recv(fd,&c,1,0);
        if(n>0){
            if(c=='\r') continue;
            buf[i++]=c;
        }else{
            return n;
        }
    }

    buf[i]=='\0';
    return n;
}

int diff_user_pass(const char *user,const char *pass){
    for(int i=0;users[i].username[0];i++){
        if(strcmp(users[i].password,pass)==0  &&  strcmp(users[i].username,user)==0){
            return 1;
        }
    }
    return 0;

}














