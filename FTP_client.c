#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 2100
#define BUF_SIZE 1024

// 读取服务器响应，返回状态码
int read_response(int sockfd, char *response, size_t size) {
    ssize_t n = read(sockfd, response, size - 1);
    if (n <= 0) return -1;
    response[n] = '\0';
    printf("Server: %s", response);
    return atoi(response);
}

// 连接到服务器控制端口
int connect_control() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return -1; }

    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

// 发送命令并处理响应
int send_command(int sockfd, const char *cmd, char *response, size_t resp_size) {
    if (write(sockfd, cmd, strlen(cmd)) < 0) {
        perror("write");
        return -1;
    }
    return read_response(sockfd, response, resp_size);
}

// 解析PASV响应，返回数据端口
int parse_pasv_response(const char *response, char *ip, size_t ip_size) {
    int h1,h2,h3,h4,p1,p2;
    const char *p = strchr(response, '(');
    if (!p || sscanf(p, "(%d,%d,%d,%d,%d,%d)", &h1,&h2,&h3,&h4,&p1,&p2) != 6)
        return -1;
    snprintf(ip, ip_size, "%d.%d.%d.%d", h1,h2,h3,h4);
    return p1*256 + p2;
}

// 建立数据连接（PASV）
int establish_data_connection(int ctrl_sock) {
    char response[BUF_SIZE];
    if (send_command(ctrl_sock, "PASV\r\n", response, sizeof(response)) != 227) {
        fprintf(stderr, "PASV failed\n");
        return -1;
    }

    char data_ip[32];
    int data_port = parse_pasv_response(response, data_ip, sizeof(data_ip));
    if (data_port < 0) {
        fprintf(stderr, "Invalid PASV response\n");
        return -1;
    }

    int data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (data_sock < 0) { perror("socket"); return -1; }

    struct sockaddr_in dataaddr = {0};
    dataaddr.sin_family = AF_INET;
    dataaddr.sin_port = htons(data_port);
    if (inet_pton(AF_INET, data_ip, &dataaddr.sin_addr) <= 0) {
        perror("inet_pton");
        close(data_sock);
        return -1;
    }

    if (connect(data_sock, (struct sockaddr*)&dataaddr, sizeof(dataaddr)) < 0) {
        perror("connect data");
        close(data_sock);
        return -1;
    }
    return data_sock;
}

// 文件列表（LIST命令）
int ftp_list(int ctrl_sock, const char *path) {
    char response[BUF_SIZE], cmd[BUF_SIZE];
    int data_sock = establish_data_connection(ctrl_sock);
    if (data_sock < 0) return -1;

    snprintf(cmd, sizeof(cmd), "LIST %s\r\n", strlen(path) ? path : "");
    if (send_command(ctrl_sock, cmd, response, sizeof(response)) != 150) {
        fprintf(stderr, "LIST failed\n");
        close(data_sock);
        return -1;
    }

    // 接收数据
    ssize_t n;
    while ((n = read(data_sock, response, BUF_SIZE-1)) > 0) {
        response[n] = '\0';
        printf("%s", response);
    }

    close(data_sock);
    read_response(ctrl_sock, response, sizeof(response)); // 226响应
    return 0;
}

// 文件下载
int ftp_retr(int ctrl_sock, const char *filename) {
    char response[BUF_SIZE], cmd[BUF_SIZE];
    int data_sock = establish_data_connection(ctrl_sock);
    if (data_sock < 0) return -1;

    snprintf(cmd, sizeof(cmd), "RETR %s\r\n", filename);
    if (send_command(ctrl_sock, cmd, response, sizeof(response)) != 150) {
        fprintf(stderr, "RETR failed\n");
        close(data_sock);
        return -1;
    }
    char want_path[BUF_SIZE];
    printf("请输入你想存储的位置：\n");
    //scanf("%s",&want_path);
    if (fgets(want_path, sizeof(want_path), stdin)) {
        // 去除换行符
        want_path[strcspn(want_path, "\r\n")] = '\0';
    }

    char right_filepath[BUF_SIZE];
    //存储到当前目录
    const char* last_slash = strrchr(filename, '/');
    snprintf(right_filepath,BUF_SIZE,"%s/%s",want_path,last_slash+1);


    //printf("%s\n",right_filepath);

    //printf("aaaaaa\n");

    FILE *fp = fopen(right_filepath, "wb");
    if (!fp) { perror("fopen"); close(data_sock); return -1; }
    //printf("bbbbbbbb\n");
    
    /*ssize_t n;
    while ((n = read(data_sock, response, BUF_SIZE)) > 0) {
        if (fwrite(response, 1, n, fp) != n) {
            perror("fwrite");
            break;
        }
    }*/
    char buf[BUF_SIZE]={0};
    ssize_t n;
    while ((n=recv(data_sock,buf,sizeof(buf),0))>0)
    {
        printf("%s\n",buf);
        
        if (fwrite(buf, 1, n, fp) != n) {
            perror("fwrite");
            break;
        }
    }
    
    //printf("ccccccc\n");
    
    fclose(fp);
    close(data_sock);
    read_response(ctrl_sock, response, sizeof(response)); // 226响应
    return  0;
}

// 文件上传
int ftp_stor(int ctrl_sock, const char *filename) {
    char response[BUF_SIZE], cmd[BUF_SIZE];
    int data_sock = establish_data_connection(ctrl_sock);
    if (data_sock < 0) return -1;

    snprintf(cmd, sizeof(cmd), "STOR %s\r\n", filename);
    if (send_command(ctrl_sock, cmd, response, sizeof(response)) != 150) {
        fprintf(stderr, "STOR failed\n");
        close(data_sock);
        return -1;
    }
    
    //printf("filename:%s\n",filename);
    FILE *fp = fopen(filename, "rb");
    if (!fp) { perror("fopen"); close(data_sock); return -1; }
    //printf("aaaaaa\n");
    
    ssize_t n;
    while ((n = fread(response, 1, BUF_SIZE, fp)) > 0) {
        if (write(data_sock, response, n) != n) {
            perror("write");
            break;
        }
    }
    //printf("bbbbbbbb\n");
    
    fclose(fp);
    close(data_sock);
    read_response(ctrl_sock, response, sizeof(response)); // 226响应
    return  0;
}

int main() {
    int ctrl_sock = connect_control();
    if (ctrl_sock < 0) {
        fprintf(stderr, "控制连接失败\n");
        return 1;
    }

    char response[BUF_SIZE];
    if (read_response(ctrl_sock, response, sizeof(response)) != 220) {
        fprintf(stderr, "欢迎消息错误\n");
        close(ctrl_sock);
        return 1;
    }

    // 用户认证
    char username[64], password[64];
    printf("Username: ");
    if (!fgets(username, sizeof(username), stdin)) { close(ctrl_sock); return 1; }
    username[strcspn(username, "\r\n")] = 0;
    snprintf(response, BUF_SIZE, "USER %s\r\n", username);
    if (send_command(ctrl_sock, response, response, sizeof(response)) != 331) {
        fprintf(stderr, "用户认证失败\n");
        close(ctrl_sock);
        return 1;
    }

    printf("Password: ");
    if (!fgets(password, sizeof(password), stdin)) { close(ctrl_sock); return 1; }
    password[strcspn(password, "\r\n")] = 0;
    snprintf(response, BUF_SIZE, "PASS %s\r\n", password);
    if (send_command(ctrl_sock, response, response, sizeof(response)) != 230) {
        fprintf(stderr, "密码错误\n");
        close(ctrl_sock);
        return 1;
    }

    printf("登录成功！\n");

    // 命令循环
    while (1) {
        printf("\n选项:\n1.列表\n2.下载\n3.上传\n4.退出\n输入选择: ");
        char input[32];
        if (!fgets(input, sizeof(input), stdin)) break;

        switch (atoi(input)) {
            case 1: {
                printf("输入目录路径（空为当前目录）: ");
                char path[256];
                fgets(path, sizeof(path), stdin);
                path[strcspn(path, "\r\n")] = 0;
                ftp_list(ctrl_sock, path);
                break;
            }
            case 2: {
                printf("输入要下载的文件名: ");
                char fname[256];
                fgets(fname, sizeof(fname), stdin);
                fname[strcspn(fname, "\r\n")] = 0;
                /*printf("输入要下载的文件名: ");
                char fpath[256];
                fgets(fpath, sizeof(fpath), stdin);
                fpath[strcspn(fpath, "\r\n")] = 0;*/
                ftp_retr(ctrl_sock, fname);
                break;
            }
            case 3: {
                printf("输入要上传的文件名: ");
                char fname[256];
                fgets(fname, sizeof(fname), stdin);
                fname[strcspn(fname, "\r\n")] = 0;
                ftp_stor(ctrl_sock, fname);
                break;
            }
            case 4: {
                send_command(ctrl_sock, "QUIT\r\n", response, sizeof(response));
                close(ctrl_sock);
                printf("连接已关闭\n");
                return 0;
            }
            default:
                printf("无效选项\n");
        }
    }

    close(ctrl_sock);
    return 0;
}