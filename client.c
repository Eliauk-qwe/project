#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"    // 修改为服务器IP
#define SERVER_PORT 2100         // 控制端口
#define BUF_SIZE 1024

// 读取服务器响应，返回第一个数字状态码
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
    inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

// 发送命令并读取响应
int send_command(int sockfd, const char *cmd, char *response, size_t resp_size) {
    if (write(sockfd, cmd, strlen(cmd)) < 0) {
        perror("write");
        return -1;
    }
    return read_response(sockfd, response, resp_size);
}

// 解析PASV响应，提取数据端口
// 格式: 227 entering passive mode (h1,h2,h3,h4,p1,p2)
int parse_pasv_response(const char *response, char *ip, size_t ip_size) {
    int h1,h2,h3,h4,p1,p2;
    const char *p = strchr(response, '(');
    if (!p) return -1;
    sscanf(p, "(%d,%d,%d,%d,%d,%d)", &h1,&h2,&h3,&h4,&p1,&p2);
    snprintf(ip, ip_size, "%d.%d.%d.%d", h1,h2,h3,h4);
    return p1*256 + p2;
}

// 连接数据端口
int connect_data(const char *ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return -1; }
    struct sockaddr_in dataaddr = {0};
    dataaddr.sin_family = AF_INET;
    dataaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &dataaddr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&dataaddr, sizeof(dataaddr)) < 0) {
        perror("connect data");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

// 列出文件列表（LIST命令）
int ftp_list(int ctrl_sock, const char *path) {
    char response[BUF_SIZE];
    char cmd[BUF_SIZE];  // 增大缓冲区以容纳路径

    // 发送 PASV 命令
    if (send_command(ctrl_sock, "PASV\r\n", response, sizeof(response)) != 227) {
        fprintf(stderr, "PASV failed\n");
        return -1;
    }

    // 解析数据端口
    char data_ip[32];
    int data_port = parse_pasv_response(response, data_ip, sizeof(data_ip));
    if (data_port < 0) {
        fprintf(stderr, "Parse PASV failed\n");
        return -1;
    }

    // 连接数据端口
    int data_sock = connect_data(data_ip, data_port);
    if (data_sock < 0) return -1;

    // 发送带路径的 LIST 命令（例如 "LIST /home/user\r\n"）
    if (strlen(path) == 0) {
        snprintf(cmd, sizeof(cmd), "LIST\r\n");
    } else {
        snprintf(cmd, sizeof(cmd), "LIST %s\r\n", path);
    }

    if (send_command(ctrl_sock, cmd, response, sizeof(response)) != 150) {
        fprintf(stderr, "LIST command failed\n");
        close(data_sock);
        return -1;
    }

    // 读取数据连接传来的文件列表
    ssize_t n;
    while ((n = read(data_sock, response, sizeof(response) - 1)) > 0) {
        response[n] = '\0';
        printf("%s", response);
    }

    close(data_sock);
    // 读取完成响应
    read_response(ctrl_sock, response, sizeof(response));
    return 0;
}
// 下载文件（RETR命令）
int ftp_retr(int ctrl_sock, const char *filename) {
    char response[BUF_SIZE];
    char cmd[256];

    // 进入被动模式
    snprintf(cmd, sizeof(cmd), "PASV\r\n");
    if (send_command(ctrl_sock, cmd, response, sizeof(response)) != 227) {
        fprintf(stderr, "PASV failed\n");
        return -1;
    }

    char data_ip[32];
    int data_port = parse_pasv_response(response, data_ip, sizeof(data_ip));
    if (data_port < 0) {
        fprintf(stderr, "Parse PASV failed\n");
        return -1;
    }

    int data_sock = connect_data(data_ip, data_port);
    if (data_sock < 0) return -1;

    snprintf(cmd, sizeof(cmd), "RETR %s\r\n", filename);
    if (send_command(ctrl_sock, cmd, response, sizeof(response)) != 150) {
        fprintf(stderr, "RETR command failed\n");
        close(data_sock);
        return -1;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("fopen");
        close(data_sock);
        return -1;
    }

    ssize_t n;
    while ((n = read(data_sock, response, sizeof(response))) > 0) {
        fwrite(response, 1, n, fp);
    }
    fclose(fp);
    close(data_sock);

    // 读取传输完成响应
    read_response(ctrl_sock, response, sizeof(response));
    return 0;
}

// 上传文件（STOR命令）
int ftp_stor(int ctrl_sock, const char *filename) {
    char response[BUF_SIZE];
    char cmd[256];

    // 进入被动模式
    snprintf(cmd, sizeof(cmd), "PASV\r\n");
    if (send_command(ctrl_sock, cmd, response, sizeof(response)) != 227) {
        fprintf(stderr, "PASV failed\n");
        return -1;
    }

    char data_ip[32];
    int data_port = parse_pasv_response(response, data_ip, sizeof(data_ip));
    if (data_port < 0) {
        fprintf(stderr, "Parse PASV failed\n");
        return -1;
    }

    int data_sock = connect_data(data_ip, data_port);
    if (data_sock < 0) return -1;

    snprintf(cmd, sizeof(cmd), "STOR %s\r\n", filename);
    if (send_command(ctrl_sock, cmd, response, sizeof(response)) != 150) {
        fprintf(stderr, "STOR command failed\n");
        close(data_sock);
        return -1;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        close(data_sock);
        return -1;
    }

    ssize_t n;
    while ((n = fread(response, 1, sizeof(response), fp)) > 0) {
        if (write(data_sock, response, n) != n) {
            perror("write data");
            fclose(fp);
            close(data_sock);
            return -1;
        }
    }

    fclose(fp);
    close(data_sock);

    // 读取传输完成响应
    read_response(ctrl_sock, response, sizeof(response));
    return 0;
}

int main() {
    int ctrl_sock = connect_control();
    if (ctrl_sock < 0) {
        fprintf(stderr, "Failed to connect control socket\n");
        return 1;
    }

    char response[BUF_SIZE];
    // 读取欢迎信息
    if (read_response(ctrl_sock, response, sizeof(response)) != 220) {
        fprintf(stderr, "Did not receive welcome message\n");
        close(ctrl_sock);
        return 1;
    }

    // 用户登录
    char user_cmd[64], pass_cmd[64];
    printf("Username: ");
    char username[64], password[64];
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\r\n")] = 0; // 去掉换行

    snprintf(user_cmd, sizeof(user_cmd), "USER %s\r\n", username);
    if (send_command(ctrl_sock, user_cmd, response, sizeof(response)) != 331) {
        fprintf(stderr, "USER command failed\n");
        close(ctrl_sock);
        return 1;
    }

    printf("Password: ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\r\n")] = 0;

    snprintf(pass_cmd, sizeof(pass_cmd), "PASS %s\r\n", password);
    if (send_command(ctrl_sock, pass_cmd, response, sizeof(response)) != 230) {
        fprintf(stderr, "PASS command failed\n");
        close(ctrl_sock);
        return 1;
    }

    printf("Login successful!\n");

    // 简单交互菜单
    while (1) {
        printf("\nCommands:\n");
        printf("1. LIST\n");
        printf("2. RETR <filename>\n");
        printf("3. STOR <filename>\n");
        printf("4. QUIT\n");
        printf("Enter choice: ");

        char choice[128];
        if (!fgets(choice, sizeof(choice), stdin)) break;
        int cmd = atoi(choice);

        // 修改后的菜单处理逻辑
if (cmd == 1) {  // LIST 命令
    printf("Enter directory path (leave empty for current dir): ");
    char path[256];
    if (!fgets(path, sizeof(path), stdin)) break;
    path[strcspn(path, "\r\n")] = 0;  // 去掉换行符
    ftp_list(ctrl_sock, path);        // 传递路径参数
} else if (cmd == 2) {  // RETR 命令
    printf("Enter filename to download: ");
    char fname[256];
    if (!fgets(fname, sizeof(fname), stdin)) break;
    fname[strcspn(fname, "\r\n")] = 0;
    ftp_retr(ctrl_sock, fname);
} else if (cmd == 3) {  // STOR 命令
    printf("Enter filename to upload: ");
    char fname[256];
    if (!fgets(fname, sizeof(fname), stdin)) break;
    fname[strcspn(fname, "\r\n")] = 0;
    ftp_stor(ctrl_sock, fname);
} else if (cmd == 4) {  // QUIT 命令
    send_command(ctrl_sock, "QUIT\r\n", response, sizeof(response));
    close(ctrl_sock);
    exit(0);  // 保证不再继续循环
}
    }

    close(ctrl_sock);
    return 0;
}
