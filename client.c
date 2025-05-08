#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define CONTROL_PORT 2100
#define BUFFER_SIZE 1024

// 连接到FTP服务器并发送命令
int connect_to_server(const char *server_ip) {
    int control_socket;
    struct sockaddr_in server_addr;

    control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (control_socket < 0) {
        perror("Error creating control socket");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(CONTROL_PORT);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(control_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        close(control_socket);
        return -1;
    }

    return control_socket;
}

// 发送PASV命令并解析返回的端口号
int enter_passive_mode(int control_socket) {
    char buffer[BUFFER_SIZE];
    int p1, p2, data_port;

    send(control_socket, "PASV\r\n", 6, 0);
    recv(control_socket, buffer, BUFFER_SIZE, 0);
    printf("Server response: %s", buffer);

    // 从服务器响应中提取端口号
    if (sscanf(buffer, "227 Entering Passive Mode (%*d,%*d,%*d,%*d,%d,%d)", &p1, &p2) == 2) {
        data_port = p1 * 256 + p2;
        printf("Data port: %d\n", data_port);
    } else {
        return -1;
    }

    return data_port;
}

// 启动数据连接并进行文件传输
void transfer_file(int control_socket, const char *filename, int is_upload) {
    int data_socket;
    struct sockaddr_in data_addr;
    FILE *file;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // 创建数据传输的socket
    data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket < 0) {
        perror("Error creating data socket");
        return;
    }

    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(enter_passive_mode(control_socket));  // 获取数据端口
    data_addr.sin_addr.s_addr = inet_addr("192.168.1.1");  // 使用服务器的IP

    if (connect(data_socket, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
        perror("Error connecting to data port");
        close(data_socket);
        return;
    }

    if (is_upload) {
        // 上传文件
        file = fopen(filename, "rb");
        if (file == NULL) {
            perror("Error opening file for upload");
            close(data_socket);
            return;
        }

        // 读取文件并发送数据
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
            send(data_socket, buffer, bytes_read, 0);
        }

        printf("File uploaded successfully.\n");
        fclose(file);
    } else {
        // 下载文件
        file = fopen(filename, "wb");
        if (file == NULL) {
            perror("Error opening file for download");
            close(data_socket);
            return;
        }

        // 接收数据并写入文件
        while ((bytes_read = recv(data_socket, buffer, BUFFER_SIZE, 0)) > 0) {
            fwrite(buffer, 1, bytes_read, file);
        }

        printf("File downloaded successfully.\n");
        fclose(file);
    }

    close(data_socket);
}

// 主函数
int main() {
    const char *server_ip = "192.168.1.1";
    int control_socket = connect_to_server(server_ip);
    if (control_socket == -1) return -1;

    // 发送PASV命令并获取数据端口
    int data_port = enter_passive_mode(control_socket);
    if (data_port == -1) {
        printf("Error entering passive mode\n");
        close(control_socket);
        return -1;
    }

    // 文件上传或下载
    transfer_file(control_socket, "file.txt", 1);  // 上传文件
    // transfer_file(control_socket, "file.txt", 0);  // 下载文件

    close(control_socket);
    return 0;
}