#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <dirent.h>

#define CONTROL_PORT 2100
#define BUFFER_SIZE 1024

// 用于存储客户端信息
typedef struct {
    int control_socket;
    struct sockaddr_in client_addr;
} client_info_t;

// 处理文件列表的函数
void send_file_list(int data_socket) {
    DIR *dir;
    struct dirent *entry;
    char buffer[BUFFER_SIZE];

    dir = opendir(".");
    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }

    // 读取并发送目录中的文件列表
    while ((entry = readdir(dir)) != NULL) {
        snprintf(buffer, sizeof(buffer), "%s\r\n", entry->d_name);
        send(data_socket, buffer, strlen(buffer), 0);
    }

    closedir(dir);
}

// 处理文件上传的函数
void receive_file(int data_socket, const char *filename) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    int file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_fd == -1) {
        perror("Error opening file for writing");
        return;
    }

    // 接收文件内容并写入文件
    while ((bytes_received = recv(data_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        write(file_fd, buffer, bytes_received);
    }

    close(file_fd);
    printf("File uploaded successfully.\n");
}

// 处理文件下载的函数
void send_file(int data_socket, const char *filename) {
    char buffer[BUFFER_SIZE];
    int bytes_read;
    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        perror("Error opening file for reading");
        return;
    }

    // 读取文件并发送内容
    while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        send(data_socket, buffer, bytes_read, 0);
    }

    close(file_fd);
    printf("File downloaded successfully.\n");
}

// 启动一个新线程来处理客户端连接
void *handle_client(void *client_sock) {
    int control_socket = *((int *) client_sock);
    char buffer[BUFFER_SIZE];
    char *response;

    // 发送欢迎消息
    send(control_socket, "220 Welcome to FTP server\r\n", 26, 0);

    // 进入通信循环
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(control_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }

        printf("Received command: %s", buffer);

        if (strncmp(buffer, "PASV", 4) == 0) {
            // 生成一个随机端口用于数据传输
            int data_port = rand() % 64511 + 1024;  // 1024到65535之间
            int p1 = data_port / 256;
            int p2 = data_port % 256;
            response = (char *) malloc(50);
            sprintf(response, "227 Entering Passive Mode (192,168,1,1,%d,%d)\r\n", p1, p2);
            send(control_socket, response, strlen(response), 0);
            free(response);

            // 启动一个新的线程来处理数据传输
            pthread_t data_thread;
            pthread_create(&data_thread, NULL, handle_data_connection, (void *) &data_port);
            pthread_detach(data_thread);
        } else if (strncmp(buffer, "LIST", 4) == 0) {
            // 创建数据传输线程来处理文件列表
            pthread_t data_thread;
            pthread_create(&data_thread, NULL, handle_data_connection, (void *) "LIST");
            pthread_detach(data_thread);
        } else if (strncmp(buffer, "STOR", 4) == 0) {
            // 创建数据传输线程来处理文件上传
            pthread_t data_thread;
            pthread_create(&data_thread, NULL, handle_data_connection, (void *) "STOR");
            pthread_detach(data_thread);
        } else if (strncmp(buffer, "RETR", 4) == 0) {
            // 创建数据传输线程来处理文件下载
            pthread_t data_thread;
            pthread_create(&data_thread, NULL, handle_data_connection, (void *) "RETR");
            pthread_detach(data_thread);
        } else {
            send(control_socket, "500 Unknown command\r\n", 21, 0);
        }
    }

    close(control_socket);
    return NULL;
}

// 处理数据传输连接
void *handle_data_connection(void *data_port_ptr) {
    int data_socket;
    struct sockaddr_in data_addr;
    const char *command = (const char *) data_port_ptr;

    // 创建数据传输socket
    data_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (data_socket == -1) {
        perror("Error creating data socket");
        return NULL;
    }

    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(2101);  // 示例数据端口
    data_addr.sin_addr.s_addr = inet_addr("192.168.1.1");

    if (connect(data_socket, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
        perror("Error connecting to data port");
        return NULL;
    }

    if (strcmp(command, "LIST") == 0) {
        send_file_list(data_socket);
    } else if (strcmp(command, "STOR") == 0) {
        receive_file(data_socket, "uploaded_file.txt");
    } else if (strcmp(command, "RETR") == 0) {
        send_file(data_socket, "downloaded_file.txt");
    }

    close(data_socket);
    return NULL;
}

// 启动FTP服务器
void start_ftp_server() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t client_thread;

    // 创建服务器socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(CONTROL_PORT);

    // 绑定并监听
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding server socket");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Error listening on server socket");
        exit(EXIT_FAILURE);
    }

    printf("FTP Server started on port %d...\n", CONTROL_PORT);

    // 接受客户端连接
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("Error accepting client connection");
            continue;
        }

        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

        // 启动新线程来处理客户端
        pthread_create(&client_thread, NULL, handle_client, (void *) &client_socket);
        pthread_detach(client_thread);
    }
}

int main() {
    start_ftp_server();
    return 0;
}