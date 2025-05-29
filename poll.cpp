#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // 创建poll监控列表
    struct pollfd fds[MAX_CLIENTS + 1]; // +1 for server socket
    int nfds = 1; // 当前监控的描述符数量（从服务器socket开始）
    
    // 初始化pollfd数组
    for (int i = 0; i < MAX_CLIENTS + 1; i++) {
        fds[i].fd = -1;        // 初始化为无效
        fds[i].events = 0;     // 不监控任何事件
        fds[i].revents = 0;    // 无事件发生
    }

    // 创建服务器socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 设置端口重用
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定端口
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 开始监听
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);
    
    // 添加服务器socket到监控列表
    fds[0].fd = server_fd;
    fds[0].events = POLLIN; // 监控可读事件（新连接）

    while (1) {
        // 调用poll等待事件 (-1 表示无限等待)
        int ready = poll(fds, nfds, -1);
        if (ready < 0) {
            perror("poll error");
            exit(EXIT_FAILURE);
        }

        // 处理所有就绪的事件
        for (int i = 0; i < nfds; i++) {
            // 跳过无效描述符
            if (fds[i].fd == -1) continue;
            
            // 检查是否有事件发生
            if (fds[i].revents == 0) continue; // 无事件
            
            // 🔴 情况1：服务器socket就绪（新连接）
            if (fds[i].fd == server_fd) {
                if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                                       (socklen_t*)&addrlen)) < 0) {
                    perror("accept");
                } else {
                    printf("New connection: socket %d\n", new_socket);
                    
                    // 将新socket添加到poll列表
                    if (nfds < MAX_CLIENTS + 1) {
                        fds[nfds].fd = new_socket;
                        fds[nfds].events = POLLIN; // 监控可读事件
                        nfds++;
                        printf("Added new client at index %d\n", nfds-1);
                    } else {
                        printf("Max clients reached. Closing connection\n");
                        close(new_socket);
                    }
                }
            } 
            // 🔴 情况2：客户端socket就绪（有数据或断开）
            else {
                int client_fd = fds[i].fd;
                int valread = read(client_fd, buffer, BUFFER_SIZE);
                
                // 客户端断开连接
                if (valread <= 0) {
                    if (valread == 0) { // 正常关闭
                        printf("Client %d disconnected\n", client_fd);
                    } else { // 读取错误
                        perror("read error");
                    }
                    
                    close(client_fd);
                    // 从列表中移除：设置为无效
                    fds[i].fd = -1;
                    
                    // 压缩列表（可选优化）
                    // 这里为了简单，保留位置但标记为无效
                } 
                // 处理客户端数据
                else {
                    buffer[valread] = '\0';
                    printf("From client %d: %s\n", client_fd, buffer);
                    
                    // 回显服务
                    send(client_fd, buffer, strlen(buffer), 0);
                    printf("Echoed back to %d\n", client_fd);
                }
            }
        }
        
        // 可选：压缩列表（移除无效项）
        // 实际应用中推荐添加此优化
    }
    
    return 0;
}
