#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define CONTROL_PORT 2100
#define BUFFER_SIZE 1024

int parse_pasv_response(char *response, char *ip, int *port) {
    int h1, h2, h3, h4, p1, p2;
    if (sscanf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
               &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        return -1;
    }
    sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4);
    *port = p1 * 256 + p2;
    return 0;
}

int connect_to_server(const char *ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int send_command(int sockfd, const char *cmd, char *response) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s\r\n", cmd);
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("Send failed");
        return -1;
    }

    int total_len = 0;
    while (1) {
        int len = recv(sockfd, response + total_len, BUFFER_SIZE - 1 - total_len, 0);
        if (len <= 0) break;
        total_len += len;
        response[total_len] = '\0';
        if (strstr(response, "\r\n") != NULL) break;
    }
    return 0;
}

int handle_list(int control_sock) {
    char response[BUFFER_SIZE];
    char ip[64];
    int port;

    // 1. 发送 PASV 命令并解析响应
    if (send_command(control_sock, "PASV", response) < 0) return -1;
    if (parse_pasv_response(response, ip, &port) < 0) {
        fprintf(stderr, "Failed to parse PASV response\n");
        return -1;
    }

    // 2. 先连接数据端口
    int data_sock = connect_to_server(ip, port);
    if (data_sock < 0) return -1;

    // 3. 再发送 LIST 命令
    if (send_command(control_sock, "LIST", response) < 0) {
        close(data_sock);
        return -1;
    }

    // 4. 接收数据
    int len;
    while ((len = recv(data_sock, response, BUFFER_SIZE - 1, 0)) > 0) {
        response[len] = '\0';
        printf("%s", response);
    }

    close(data_sock);
    return 0;
}

int handle_retr(int control_sock, const char *filename) {
    char response[BUFFER_SIZE];
    char ip[64];
    int port;

    // 1. 发送 PASV 命令并解析响应
    if (send_command(control_sock, "PASV", response) < 0) return -1;
    if (parse_pasv_response(response, ip, &port) < 0) {
        fprintf(stderr, "Failed to parse PASV response\n");
        return -1;
    }

    // 2. 先连接数据端口
    int data_sock = connect_to_server(ip, port);
    if (data_sock < 0) return -1;

    // 3. 再发送 RETR 命令
    char cmd[BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "RETR %s", filename);
    if (send_command(control_sock, cmd, response) < 0) {
        close(data_sock);
        return -1;
    }

    // 4. 接收文件数据
    int file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd < 0) {
        perror("File open failed");
        close(data_sock);
        return -1;
    }

    int len;
    while ((len = recv(data_sock, response, BUFFER_SIZE, 0)) > 0) {
        write(file_fd, response, len);
    }

    close(file_fd);
    close(data_sock);
    return 0;
}

int handle_stor(int control_sock, const char *filename) {
    char response[BUFFER_SIZE];
    char ip[64];
    int port;

    // 1. 发送 PASV 命令并解析响应
    if (send_command(control_sock, "PASV", response) < 0) return -1;
    if (parse_pasv_response(response, ip, &port) < 0) {
        fprintf(stderr, "Failed to parse PASV response\n");
        return -1;
    }

    // 2. 先连接数据端口
    int data_sock = connect_to_server(ip, port);
    if (data_sock < 0) return -1;

    // 3. 再发送 STOR 命令
    char cmd[BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "STOR %s", filename);
    if (send_command(control_sock, cmd, response) < 0) {
        close(data_sock);
        return -1;
    }

    // 4. 发送文件数据
    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        perror("File open failed");
        close(data_sock);
        return -1;
    }

    int len;
    while ((len = read(file_fd, response, BUFFER_SIZE)) > 0) {
        send(data_sock, response, len, 0);
    }

    close(file_fd);
    close(data_sock);
    return 0;
}

int main() {
    const char *server_ip = "127.0.0.1";
    int control_sock = connect_to_server(server_ip, CONTROL_PORT);
    if (control_sock < 0) return -1;

    char response[BUFFER_SIZE];
    recv(control_sock, response, BUFFER_SIZE - 1, 0);
    printf("%s", response);

    char cmd[BUFFER_SIZE], arg[256];
    while (1) {
        printf("ftp> ");
        fflush(stdout);

        if (fgets(cmd, sizeof(cmd), stdin) == NULL) break;
        cmd[strcspn(cmd, "\n")] = '\0';

        if (strcmp(cmd, "LIST") == 0) {
            handle_list(control_sock);
        } else if (sscanf(cmd, "RETR %255s", arg) == 1) {
            handle_retr(control_sock, arg);
        } else if (sscanf(cmd, "STOR %255s", arg) == 1) {
            handle_stor(control_sock, arg);
        } else if (strcmp(cmd, "QUIT") == 0) {
            send_command(control_sock, "QUIT", response);
            break;
        } else {
            printf("500 Unknown command\r\n");
        }
    }

    close(control_sock);
    return 0;
}