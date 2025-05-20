#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>


#define CONTROL_PORT  2100
#define LISTEN_NUM    10
#define PTHREADPOOL_SIZE 10
#define QUEUE_SIZE      64
#define BUFSIZE       1024
#define MAX_CMD_LEN 512
#define MAX_USERNAME_LEN 32
#define MAX_PASS_LEN 32
#define MAX_PATH 512


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

threadpool_t *pool=NULL;


//任务参数结构
typedef struct{
    int control_fd;
    struct sockaddr_in client_addr;
}client_task_t;


// 处理 PASV 命令，启动数据监听线程，返回端口和监听 fd
typedef struct {
    int data_listen_fd;   // 数据监听套接字描述符
    int data_port;        // 对外暴露的监听端口号
    //pthread_t data_thread; // 处理数据连接的线程ID
    int data_fd;          // 实际数据传输的连接套接字
    int ctrl_fd;          // 控制连接的套接字描述符（用于发送响应）
    char cmd[MAX_CMD_LEN];
} data_transfer_t;

// 数据传输线程参数
typedef struct {
    int data_fd;
    int ctrl_fd;
    char cmd[MAX_CMD_LEN];
} data_thread_arg_t;

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
void *pasv_thread_func(void *arg) ;
void *data_thread_func(void *arg) ;
void ftp_list(int data_fd, const char *path) ;
void ftp_stor(int ctrl_fd,int data_fd,const char *filename);
void ftp_retr(int ctrl_fd,int data_fd,const char *filename);
int threadpool_add(threadpool_t *pool,void *arg);









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
    if(!pool){
        fprintf(stderr,"creat_pthreadpool() failed");
        close(server_fd);
        return -1;
    }

    printf("FTP Server is listening %d\n",CONTROL_PORT);

    while(1){
        struct sockaddr_in client_addr;
        socklen_t len=sizeof(client_addr);
        int client_con_fd=accept(server_fd,(struct sockaddr*)&client_addr,&len);
        if(client_con_fd<0){
            perror("accept()");
            continue;
        }

        client_task_t *task=malloc(sizeof(client_task_t));
        task->client_addr=client_addr;
        task->control_fd=client_con_fd;

        if(threadpool_add(pool,task)!=0){
            fprintf(stderr,"Server is busying,reject client");
            close(client_con_fd);
            free(task);

        }

        

    }

    // 发送关闭信号
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->cond);  // 唤醒所有线程

// 等待所有线程退出
for (int i = 0; i < pool->thread_count; i++) {
    pthread_join(pool->threads[i], NULL);
}

// 释放资源
free(pool->threads);
free(pool->task_args);
free(pool);


    close(server_fd);
    return 0;



}


static int creat_listen_socket(int port){
    int fd;
    fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd<0){
        perror("socket()");
        return -1;
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
    if(bind(fd,(struct sockaddr*)&addr,len) <0){
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
        //---------------------------------------------------------------
        //---------------------------------------------------------------
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

    ftp_send(ctrl_fd,"220 Welcome to my FTP ");






    char buf[MAX_CMD_LEN];
    //必须初始化为 {0}，以确保敏感数据的安全性和确定性。
    char user[MAX_USERNAME_LEN] = {0};
    char pass[MAX_PASS_LEN] = {0};
    int flag=0;
    //初始值0表示尚未分配端口
    int pasv_port=0;
    //套接字尚未创建
    int pasv_socket=-1;
    data_transfer_t *data_t =NULL;
    pthread_t data_tid;
    


    while(1){
        int recv=ftp_recv(ctrl_fd,buf,BUFSIZE);
        if(recv<0)  break;

        //接受命令
        printf("CMD:%s",buf);


        //密码验证部分
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


        // 进入被动模式，创建监听端口
        else if(strncmp(buf,"PASV",4)==0){
            //随机生成一个端口
            //实际应用中，40000-50000常被用作非标准服务的默认端口范围
            pasv_port=rand()%10000+40000;
            pasv_socket=creat_listen_socket(pasv_port);
            if(pasv_socket<0){
                ftp_send(ctrl_fd,"425 Can't open data connection");
                continue;
            }

            //RFC 959规定PASV响应应返回与控制连接相同的服务器IP地址
            //即使数据连接使用不同端口，IP地址应与控制连接保持一致


            //获取服务器ip
            struct sockaddr_in addr;
            socklen_t len=sizeof(addr);
            getsockname(ctrl_fd,(struct sockaddr*)&addr,&len);
            unsigned char *ip=(unsigned char*)&addr.sin_addr.s_addr;

            //将生成的端口号告知客户端控制线程，返回 227 entering passive mode (h1,h2,h3,h4,p1,p2)，其中端口号为 p1*256+p2，IP 地址为 h1.h2.h3.h4。
            ftp_send(ctrl_fd,"227 entering passive mode (%u,%u,%u,%u,%u,%u)",ip[0],ip[1],ip[2],ip[3],(pasv_port >> 8) & 0xFF, pasv_port & 0xFF);
            
               
            // 等待客户端连接数据端口，启动线程接收数据连接
            data_t = malloc(sizeof(data_transfer_t));
            data_t->ctrl_fd=ctrl_fd;
            data_t->data_listen_fd=pasv_socket;
            data_t->data_port=pasv_port;
            //初始化
            data_t->data_fd=-1;

            pthread_create(&data_tid,NULL,pasv_thread_func,data_t);
            pthread_detach(data_tid);




        }else if(strncmp(buf,"QUIT",4)==0){
            ftp_send(ctrl_fd, "221 Goodbye");
        }

        else if (strncmp(buf, "LIST", 4) == 0 ||
         strncmp(buf, "RETR", 4) == 0 ||
         strncmp(buf, "STOR", 4) == 0) {
            if (data_t) {
                strcpy(data_t->cmd, buf); // <== 添加此句
                ftp_send(ctrl_fd, "150 Opening data connection."); 
            } else {
                ftp_send(ctrl_fd, "425 Use PASV first.");
                continue;
            }
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

    send(fd,buf,strlen(buf),0);
    
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

    buf[i]='\0';
    return i;
}

int diff_user_pass(const char *user,const char *pass){
    for(int i=0;users[i].username[0];i++){
        if(strcmp(users[i].password,pass)==0  &&  strcmp(users[i].username,user)==0){
            return 1;
        }
    }
    return 0;

}

void *pasv_thread_func(void *arg) {
    data_transfer_t *data_t =(data_transfer_t *)arg;
    struct sockaddr_in client_addr;
    socklen_t len=sizeof(client_addr);

    data_t -> data_fd = accept(data_t->data_listen_fd,(struct sockaddr*)&client_addr,&len);
    if(data_t->data_fd <0){
        perror("accept data");
        close(data_t->data_listen_fd);
        free(data_t);
        return NULL;
    }
    close(data_t->data_listen_fd);

    data_thread_arg_t *data_thread_t =malloc(sizeof(data_thread_arg_t));
    //data_thread_t->cmd=data_t->cmd;
    strcpy(data_thread_t->cmd,data_t->cmd);
    data_thread_t->ctrl_fd=data_t->ctrl_fd;
    data_thread_t->data_fd=data_t->data_fd;


    pthread_t tid;
    if(pthread_create(&tid,NULL,data_thread_func,data_thread_t)==0){
        pthread_detach(tid);
    }else{
        perror("pthread_create ");
        close(data_t->data_fd);
        free(data_thread_t);
    }

    free(data_t);
    return NULL;


}

void *data_thread_func(void *arg) {
    data_thread_arg_t *data_thread_t =(data_thread_arg_t*) arg;
    int data_fd=data_thread_t->data_fd;
    int ctrl_fd=data_thread_t->ctrl_fd;
    char *cmd =data_thread_t->cmd;

    if(strncmp(cmd,"LIST",4)==0){
        char path[MAX_PATH] = {0};
        if(strlen(cmd) > 5) sscanf(cmd+5, "%s", path);
        ftp_list(data_fd, path);
    }if(strncmp(cmd,"RETR",4)==0){
        char filename[MAX_PATH]={0};
        sscanf(cmd+5,"%s",filename);
        ftp_retr(ctrl_fd,data_fd,filename);
    }if(strncmp(cmd,"STOR",4)==0){
        char filename[MAX_PATH]={0};
        sscanf(cmd+5,"%s",filename);
        ftp_stor(ctrl_fd,data_fd,filename);
    }

    close(data_thread_t->data_fd);
    ftp_send(ctrl_fd, "226 Transfer complete");
    free(data_thread_t);
    return NULL;


}




void ftp_list(int data_fd, const char *path) {
    DIR *dir;
    struct dirent *entry;
    char full_path[MAX_PATH] = {0};
    char buf[BUFSIZE] = {0};
    char actual_path[MAX_PATH] = {0};

    if (path == NULL || strlen(path) == 0) {
        strncpy(actual_path, ".", MAX_PATH - 1);
    } else {
        strncpy(actual_path, path, MAX_PATH - 1);
    }

    dir = opendir(actual_path);
    if (!dir) {
        snprintf(buf, sizeof(buf), "Failed to open directory: %s\r\n", actual_path);
        send(data_fd, buf, strlen(buf), 0);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;

        snprintf(full_path, sizeof(full_path), "%s/%s", actual_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == -1) continue;

        // 文件类型和权限
        char mode_str[11];
        mode_str[0] = S_ISDIR(st.st_mode) ? 'd' : '-';
        mode_str[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
        mode_str[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
        mode_str[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
        mode_str[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
        mode_str[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
        mode_str[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
        mode_str[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
        mode_str[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
        mode_str[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';
        mode_str[10] = '\0';

        // 用户名、组名
        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        const char *uname = pw ? pw->pw_name : "unknown";
        const char *gname = gr ? gr->gr_name : "unknown";

        // 修改时间
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%b %d %H:%M", localtime(&st.st_mtime));

        // 格式化输出
        snprintf(buf, sizeof(buf), "%s 1 %s %s %10ld %s %s\r\n",
                 mode_str, uname, gname, (long)st.st_size, time_str, entry->d_name);

        send(data_fd, buf, strlen(buf), 0);
    }

    closedir(dir);
}

void ftp_stor(int ctrl_fd,int data_fd,const char *filename){
    char filepath[MAX_PATH];
    //存储到当前目录
    snprintf(filepath,sizeof(filepath),"./%s",filename);

    FILE *fp=fopen(filename,"wb");
    if(!fp){
        perror("fopen()");
        ftp_send(ctrl_fd,"550 Failed to open file for writing.");
        return;
    }
      
    ssize_t n;
    char buf[BUFSIZE];

    while((n=recv(data_fd,buf,sizeof(buf),0))>0){
        


        if(fwrite(buf,1,n,fp) !=n){
            perror("fwrite()");
            ftp_send(ctrl_fd,"451 Write error.");
            fclose(fp);
            return;

        }

    }

    fclose(fp);

    if (n < 0) {
        perror("recv");
        ftp_send(ctrl_fd, "426 Connection closed; transfer aborted.");
    } else {
        ftp_send(ctrl_fd, "226 File transfer complete.");
    }
}


void ftp_retr(int ctrl_fd,int data_fd,const char *filename){
    char fullpath[MAX_PATH]={0};
    char buf[BUFSIZE];

    if(filename[0]=='/'){
        snprintf(fullpath,sizeof(fullpath),"%s",filename);
    }else{
        snprintf(fullpath,sizeof(fullpath),"./%s",filename);
    }

    FILE *fp=fopen(fullpath,"rb");
     if (!fp) {
        snprintf(buf, sizeof(buf), "550 Failed to open file: %s\r\n", filename);
        send(ctrl_fd, buf, strlen(buf), 0);
        return;
    }

    int n;
    while((n=fread(buf,1,sizeof(buf),fp))>0){
        if(send(data_fd,buf,n,0)<0){
            perror("send file data");
            break;
        }

    }

    fclose(fp);


}


int threadpool_add(threadpool_t *pool,void *arg){
    pthread_mutex_lock(&pool->lock);
    if(pool->count==pool->queue_size){
        pthread_mutex_unlock(&pool->lock);
        return -1;
    }
    pool->task_args[pool->tail]=arg;
    pool->tail=(pool->tail+1)%pool->queue_size;
    pool->count++;
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->lock);
    return 0;

    
}


