
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
#include <errno.h>

#define CONTROL_PORT  2100
#define LISTEN_NUM    10
#define BUFSIZE       1024
#define MAX_CMD_LEN 512
#define MAX_USERNAME_LEN 32
#define MAX_PASS_LEN 32
#define MAX_PATH 512

static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t data_cond = PTHREAD_COND_INITIALIZER;



typedef struct {
    int control_fd;
    struct sockaddr_in client_addr;
} client_task_t;

typedef struct {
    int data_listen_fd;
    int data_port;
    int data_fd;
    int ctrl_fd;
    char cmd[MAX_CMD_LEN];
    int *data_fd_ptr;
} data_transfer_t;

typedef struct {
    int data_fd;
    int ctrl_fd;
    char cmd[MAX_CMD_LEN];
} data_thread_arg_t;

typedef struct {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASS_LEN];
} User;

User users[] = {
    {"test", "123456"},
    {"user", "pass"},
    {0}
};

static int creat_listen_socket(int port);
static void ftp_send(int fd, const char* fmt, ...);
static int ftp_recv(int fd, char *buf, int maxlen);
int diff_user_pass(const char *user, const char *pass);
void *pasv_thread_func(void *arg);
void *data_thread_func(void *arg);
void ftp_list(int data_fd, const char *path);
void ftp_stor(int ctrl_fd, int data_fd, const char *filepath);
void ftp_retr(int ctrl_fd, int data_fd, const char *filename);
void *control_func(void *arg);

int main() {
    srand(time(NULL));

    int server_fd = creat_listen_socket(CONTROL_PORT);
    if (server_fd < 0) {
        fprintf(stderr, "creat_listen_socket() failed!\n");
        close(server_fd);
        return -1;
    }

    printf("FTP Server is listening %d\n", CONTROL_PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_con_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (client_con_fd < 0) {
            perror("accept()");
            continue;
        }

        client_task_t *task = malloc(sizeof(client_task_t));
        task->control_fd = client_con_fd;
        task->client_addr = client_addr;

        pthread_t tid;
        if (pthread_create(&tid, NULL, control_func, task) != 0) {
            fprintf(stderr, "Failed to create thread\n");
            close(client_con_fd);
            free(task);
        } else {
            pthread_detach(tid); // 分离线程避免资源泄漏
        }
    }

    close(server_fd);
    return 0;
}

static int creat_listen_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket()");
        return -1;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    //无论主机字节序（小端或大端）如何，0 的二进制表示始终是全零，因此：不用htonl
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);


    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind()");
        close(fd);
        return -1;
    }

    if (listen(fd, LISTEN_NUM) < 0) {
        perror("listen()");
        close(fd);
        return -1;
    }

    return fd;
}

void *control_func(void *arg) {
    client_task_t *task = (client_task_t*)arg;
    int ctrl_fd = task->control_fd;
    free(task);

    ftp_send(ctrl_fd, "220 Welcome to my FTP server");

    char buf[MAX_CMD_LEN];
    char user[MAX_USERNAME_LEN] = {0};
    char pass[MAX_PASS_LEN] = {0};
    int flag = 0;
    int pasv_port = 0;
    int pasv_socket = -1;
    int data_fd=-1;

    while (1) {
        //printf("1\n");
        int recv=ftp_recv(ctrl_fd,buf,BUFSIZE);
        //if(recv<0)  break;
        //printf("2\n");

        if (recv <= 0) break;

        printf("CMD: %s", buf);

        if (strncmp(buf, "USER", 4) == 0) {
            sscanf(buf + 5, "%s", user);
            ftp_send(ctrl_fd, "331 Username OK, need password");
        } 
        else if (strncmp(buf, "PASS", 4) == 0) {
            sscanf(buf + 5, "%s", pass);
            if (diff_user_pass(user, pass)) {
                flag = 1;
                ftp_send(ctrl_fd, "230 Login successful");
            } else {
                ftp_send(ctrl_fd, "530 Login incorrect");
                break;
            }
        } 
        else if (!flag) {
            ftp_send(ctrl_fd, "530 Please login with USER and PASS");
        } 
        else if (strncmp(buf, "PASV", 4) == 0) {
            pasv_port = rand() % 10000 + 40000;
            pasv_socket = creat_listen_socket(pasv_port);
            if (pasv_socket < 0) {
                ftp_send(ctrl_fd, "425 Can't open data connection");
                continue;
            }

            struct sockaddr_in addr;
            socklen_t len = sizeof(addr);
            getsockname(ctrl_fd, (struct sockaddr*)&addr, &len);
            unsigned char *ip = (unsigned char*)&addr.sin_addr.s_addr;

            ftp_send(ctrl_fd, "227 Entering Passive Mode (%u,%u,%u,%u,%u,%u)",
                    ip[0], ip[1], ip[2], ip[3],
                    (pasv_port >> 8) & 0xFF, pasv_port & 0xFF);

            data_transfer_t *data_t = malloc(sizeof(data_transfer_t));
            data_t->ctrl_fd = ctrl_fd;
            data_t->data_listen_fd = pasv_socket;
            data_t->data_port = pasv_port;
            data_t->data_fd = -1;
            strcpy(data_t->cmd,buf);
            data_t->data_fd_ptr = &data_fd;

            pthread_t data_tid;
            pthread_create(&data_tid, NULL, pasv_thread_func, data_t);
            pthread_detach(data_tid);



        } 
        else if (strncmp(buf, "QUIT", 4) == 0) {
            ftp_send(ctrl_fd, "221 Goodbye");
            break;
        } 
        else if (strncmp(buf, "LIST", 4) == 0 ||
                 strncmp(buf, "RETR", 4) == 0 ||
                 strncmp(buf, "STOR", 4) == 0) {

            pthread_mutex_lock(&data_mutex);
            while(data_fd==-1){
                pthread_cond_wait(&data_cond,&data_mutex);
            }
            int curr_data_fd=data_fd;
            data_fd=-1;
            pthread_mutex_unlock(&data_mutex);

            
            data_thread_arg_t *data_thread_t=malloc(sizeof(data_thread_arg_t));
            data_thread_t->ctrl_fd=ctrl_fd;
            data_thread_t->data_fd=curr_data_fd;
            strcpy(data_thread_t->cmd,buf);
            
            printf("data_thread_t->cmd:%s\n",data_thread_t->cmd);

            pthread_t tid;
            if(pthread_create(&tid,NULL,data_thread_func,data_thread_t)==0)
                pthread_detach(tid);
            else{
                perror("pthread_create");
                close(data_fd);
                free(data_thread_t);
            }
            ftp_send(ctrl_fd, "150 Opening data connection");
        }
    }

    close(ctrl_fd);
    return NULL;
}

static void ftp_send(int fd, const char *fmt, ...) {
    char buf[BUFSIZE];
    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buf, sizeof(buf), fmt, arg);
    va_end(arg);
    strcat(buf, "\r\n");
    send(fd, buf, strlen(buf), 0);
}

/*static int ftp_recv(int fd, char *buf, int maxlen) {
//    int i = 0;
    int c;
    c = recv(fd, buf, 1, 0);
        if (c == '\r') continue;
        if (c == '\n') break;
        i++;
    buf[c] = '\0';
    return c;
}*/

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

int diff_user_pass(const char *user, const char *pass) {
    for (int i = 0; users[i].username[0]; i++) {
        if (strcmp(users[i].username, user) == 0 &&
            strcmp(users[i].password, pass) == 0) {
            return 1;
        }
    }
    return 0;
}

/*void *pasv_thread_func(void *arg){

}*/

void *pasv_thread_func(void *arg) {
    data_transfer_t *data_t = (data_transfer_t*)arg;
    struct sockaddr_in client_addr;
    socklen_t c_len = sizeof(client_addr);
    
    // 阻塞等待客户端连接
    int data_fd = accept(data_t->data_listen_fd, (struct sockaddr*)&client_addr, &c_len);
    close(data_t->data_listen_fd);

    if (data_fd >= 0) {
        // 通过指针更新control_func中的data_fd
        *(data_t->data_fd_ptr) = data_fd;
    }

    // 更新共享变量并通知条件变量
    pthread_mutex_lock(&data_mutex);
    data_fd = data_fd; // 全局变量或通过结构体传递
    pthread_cond_signal(&data_cond);
    pthread_mutex_unlock(&data_mutex);

    free(data_t);
    return NULL;
}


void *data_thread_func(void *arg) {
    //printf("data_thread_func\n");
    data_thread_arg_t *data_thread_t = (data_thread_arg_t*)arg;
    //printf("a\n");
    int data_fd = data_thread_t->data_fd;
    //printf("b\n");

    int ctrl_fd = data_thread_t->ctrl_fd;
    //printf("c\n");

    //printf("%s\n",data_thread_t->cmd);

    char cmd[MAX_CMD_LEN];
    strcpy(cmd,data_thread_t->cmd);
    //printf("d\n");


    //printf("cmd:%s",cmd);


    



    if (strncmp(cmd, "LIST",4) ==0) {
        printf("list\n");
        char path[MAX_PATH] = {0};
        if (strlen(cmd) > 5) sscanf(cmd + 5, "%s", path);
        ftp_list(data_fd, path);
    } 
    else if (strncmp(cmd, "RETR", 4) == 0) {
        char filename[MAX_PATH] = {0};
        sscanf(cmd + 5, "%s", filename);
        
        ftp_retr(ctrl_fd, data_fd, filename);
    } 
    else if (strncmp(cmd, "STOR", 4) == 0) {
        char filepath[MAX_PATH] = {0};
        sscanf(cmd + 5, "%s", filepath);
        ftp_stor(ctrl_fd, data_fd, filepath);
    }

    //close(data_thread_t->data_fd);
    ftp_send(ctrl_fd, "226 Transfer complete");
    free(data_thread_t);
    return NULL;
}

void ftp_list(int data_fd, const char *path) {

    printf("ftp_list\n");
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

        //printf("进入\n");
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

        printf("list : %s \n",buf);
        send(data_fd, buf, strlen(buf), 0);
    }

    closedir(dir);
    close(data_fd);
}

void ftp_stor(int ctrl_fd,int data_fd,const char *filepath){
    char right_filepath[MAX_PATH];
    //存储到当前目录
    const char* last_slash = strrchr(filepath, '/');
    snprintf(right_filepath,MAX_PATH,"./%s",last_slash+1);
    printf("filepath:%s\n",filepath);
    printf("right_filepath:%s\n",right_filepath);
    FILE *fp=fopen(right_filepath,"wb");
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
    //char fullpath[MAX_PATH]={0};
    char buf[BUFSIZE];

   /* if(filename[0]=='/'){
        snprintf(fullpath,sizeof(fullpath),"%s",filename);
    }else{
        snprintf(fullpath,sizeof(fullpath),"./%s",filename);
    }*/
    //printf("ftp_retr\n");
   // printf("filename:%s\n",filename);
    FILE *fp=fopen(filename,"rb");
     if (!fp) {
        snprintf(buf, sizeof(buf), "550 Failed to open file: %s\r\n", filename);
        send(ctrl_fd, buf, strlen(buf), 0);
        return;
    }
    //printf("aaaaaa\n");
    int n;
    while((n=fread(buf,1,sizeof(buf),fp))>0){
        printf("%s\n",buf);
        if(send(data_fd,buf,n,0)<0){
            perror("send file data");
            break;
        }

    }
   // printf("bbbbbbbb\n");
    
    fclose(fp);
    close(data_fd);


}