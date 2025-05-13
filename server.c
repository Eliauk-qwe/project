#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>



#define PORT 2100
#define LISTEN_NUM  5
#define BUFFER_SIZE 1024


//客户端结构体
typedef struct{
    int control_socket;
    struct sockaddr_in client_addr; 
}client_ifm;

// 数据连接结构
typedef struct {
    int data_socket;
    char command[16];
    char filename[256];
} data_conn_t;


void *control_thread_func(void *arg) ;

void *data_thread_func(void *arg) ;

void send_file_list(int data_socket) ;

void receive_file(int data_socket, const char *filename) ;

void send_file(int data_socket, const char *filename) ;

int main(){
    int server_socket;
    struct sockaddr_in server_addr ;   //注意头文件

    //创建套接字
    server_socket=socket(AF_INET,SOCK_STREAM,0);
    
    //bind分配
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);

    bind(server_socket,(struct sockaddr_in*)&server_addr,sizeof(server_addr));
    
    
    //listen监听
    listen(server_socket,LISTEN_NUM);
    printf("服务端正在监听%d端口",PORT);

    while(1){
        struct sockaddr_in client_addr;

        int client_socket=accept(server_socket,(struct sockaddr*)&client_addr,sizeof(client_addr));
        
        client_ifm *c_ifm = malloc(sizeof(client_ifm));
        c_ifm->client_addr=client_addr;
        c_ifm->control_socket=client_socket;

        pthread_t tid;
        pthread_create(&tid,NULL,control_thread_func,c_ifm);
        pthread_detach(tid);
    }

    close(server_socket);

    return 0;
}

//服务端的控制线程
void *control_thread_func(void *arg) {
    char *command,cmd,arg1;


    client_ifm *c_ifm =(client_ifm *)arg;
    int control_thread_socket=c_ifm->control_socket;
    free(c_ifm);

    const char* server_ready = "Server is ready !\r\n";
    send(control_thread_socket,server_ready,strlen(server_ready),0);
    int data_port=0;
    int pasv_socket;

    while(1){
        memset(&command,0,sizeof(command));
        recv(control_thread_socket,command,sizeof(command),0);

        //解析传过来的参数
        sscanf(command,"%s %s",cmd,arg1);
        printf("cmd:%s  arg1:%s",cmd,arg1);

        //PASV ???
        if(strcmp(cmd,"PASV")==0){
            pasv_socket=socket(AF_INET,SOCK_STREAM,0);

            struct sockaddr_in pasv_socket_addr;
            memset(&pasv_socket_addr,0,sizeof(pasv_socket_addr));
            pasv_socket_addr.sin_addr.s_addr=htonl(INADDR_ANY);
            pasv_socket_addr.sin_family=AF_INET;
            pasv_socket_addr.sin_port=0;
            bind(pasv_socket,(struct sockaddr_in *)&pasv_socket_addr,sizeof(pasv_socket_addr));

            listen(pasv_socket,1);
            //获取实际端口
            socklen_t pasv_socket_addr_len=sizeof(pasv_socket_addr);

            getsockname(pasv_socket,(struct sockaddr_in *)&pasv_socket_addr,&pasv_socket_addr_len);

            //传回信息
            data_port=ntohs(pasv_socket_addr.sin_port);
            int p1=data_port/256,p2=data_port%256;
            const char* return_ip=ntohl(pasv_socket_addr.sin_addr.s_addr);
            char *return_ifm;
            sprintf(return_ifm,"227 Entering Passive Mode (%s,%d,%d)\r\n",return_ip,p1,p2);
            send(pasv_socket,return_ifm,strlen(return_ifm),0);

        } else if (strcmp(cmd, "LIST") == 0 || strcmp(cmd, "RETR") == 0 || strcmp(cmd, "STOR") == 0) {
            struct sockaddr_in  client_data_addr;
            socklen_t client_data_addr_len=sizeof(client_data_addr);
            int client_data_socket=accept(pasv_socket,(struct sockaddr_in*)&client_data_addr,&client_data_addr_len);

            data_conn_t *con=malloc(sizeof(data_conn_t));
            strcpy(con->command,cmd);
            strcpy(con->filename,arg1);
            con->data_socket=client_data_socket;


            pthread_t tid;
            pthread_create(&tid, NULL, data_thread_func, con);
            pthread_detach(tid);

            close(pasv_socket);

        }else{
            send(control_thread_socket,"usage err\r\n",12,0);
        }
    }

    close(control_thread_socket);
    return NULL;
}


void *data_thread_func(void *arg) {
    data_conn_t * data=(data_conn_t*)arg;
    int data_job_socket=data->data_socket;

     if (strcmp(data->command, "LIST") == 0) {
        send_file_list(data_job_socket);
    } else if (strcmp(data->command, "STOR") == 0) {
        receive_file(data_job_socket, data->filename);
    } else if (strcmp(data->command, "RETR") == 0) {
        send_file(data_job_socket, data->filename);
    }

    close(data_job_socket);
    free(data);
    return NULL;
}


//LIST
void send_file_list(int data_socket) {
    DIR *dir;
    struct  dirent *entry;
    char buffer [BUFFER_SIZE];

    dir =opendir(".");   // "."表示当前目录
    if(dir == NULL){
        perror("opendir");
        return ;
    }

    while ((entry=readdir(dir)) != NULL)
    {
        snprintf(buffer,sizeof(buffer),"%s\r\n",entry->d_name);
        send(data_socket,buffer,strlen(buffer),0);
    }

    closedir(dir);
    
}

void receive_file(int data_socket, const char *filename) {
    char buffer[BUFFER_SIZE];
    int fd=open(filename,O_WRONLY | O_CREAT | O_TRUNC ,0644);
    if (fd<0)
    {
        perror("open for write");
        return ;
    }
    
    ssize_t n;
    while (n=recv(data_socket,buffer,BUFFER_SIZE,0))
    {
        write(fd,buffer,n);
    }

    close(fd);
    
}

void send_file(int data_socket, const char *filename) {
    char buffer[BUFFER_SIZE];
    int fd=open(filename,O_RDONLY );
    if (fd<0)
    {
        perror("open for read");
        return ;
    }
    
    ssize_t n;
    while ((n=read(fd,buffer,BUFFER_SIZE))>0)
    {
        send(data_socket,buffer,n,0);
    }

    close(fd);
}