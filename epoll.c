#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 8080
#define MAX_EVENTS 64
#define BUFFER_SIZE 1024
#define MODE_ET 1   // 边缘触发模式
#define MODE_LT 0   // 水平触发模式

// 设置文件描述符为非阻塞模式
void setnonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        exit(EXIT_FAILURE);
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        exit(EXIT_FAILURE);
    }
}

// 处理客户端数据
void handle_client_data(int client_fd, int trigger_mode) {
    char buffer[BUFFER_SIZE];
    int total_read = 0;
    int valread;
    
    printf("Handling client %d in %s mode\n", 
           client_fd, trigger_mode == MODE_ET ? "ET" : "LT");
    
    if (trigger_mode == MODE_ET) {
        // 边缘触发模式：必须一次性读取所有可用数据
        while (1) {
            valread = read(client_fd, buffer + total_read, BUFFER_SIZE - total_read);
            
            if (valread == -1) {
                // 非阻塞socket，没有数据可读了
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 处理已读取的数据
                    break;
                } else {
                    perror("read");
                    return;
                }
            } else if (valread == 0) {
                // 客户端断开连接
                printf("Client %d disconnected\n", client_fd);
                return;
            } else {
                total_read += valread;
                
                // 检查缓冲区是否已满
                if (total_read >= BUFFER_SIZE) {
                    printf("Buffer full, truncating message\n");
                    break;
                }
            }
        }
    } else {
        // 水平触发模式：可以只读取部分数据
        valread = read(client_fd, buffer, BUFFER_SIZE);
        
        if (valread == -1) {
            perror("read");
            return;
        } else if (valread == 0) {
            // 客户端断开连接
            printf("Client %d disconnected\n", client_fd);
            return;
        } else {
            total_read = valread;
        }
    }
    
    // 处理读取到的数据
    if (total_read > 0) {
        buffer[total_read] = '\0';
        printf("From client %d: %s\n", client_fd, buffer);
        
        // 回显服务
        send(client_fd, buffer, total_read, 0);
        printf("Echoed back to %d\n", client_fd);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [et|lt]\n", argv[0]);
        fprintf(stderr, "  et: Edge Triggered mode\n");
        fprintf(stderr, "  lt: Level Triggered mode\n");
        exit(EXIT_FAILURE);
    }
    
    int trigger_mode = MODE_LT; // 默认水平触发
    if (strcmp(argv[1], "et") == 0) {
        trigger_mode = MODE_ET;
        printf("Using Edge Triggered (ET) mode\n");
    } else {
        printf("Using Level Triggered (LT) mode\n");
    }
    
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // 创建epoll实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
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
    if (listen(server_fd, 128) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    // 设置服务器socket为非阻塞
    setnonblocking(server_fd);

    // 注册服务器socket到epoll
    struct epoll_event ev;
    // 服务器socket始终使用水平触发模式
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: server_fd");
        exit(EXIT_FAILURE);
    }

    // 存储事件的数组
    struct epoll_event events[MAX_EVENTS];

    while (1) {
        // 等待事件发生（-1表示无限等待）
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        printf("Epoll returned %d events\n", nfds);
        
        // 处理所有就绪的事件
        for (int i = 0; i < nfds; i++) {
            // 新客户端连接（服务器socket）
            if (events[i].data.fd == server_fd) {
                // 接受所有等待的连接
                while ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                                          (socklen_t*)&addrlen)) > 0) {
                    printf("New connection: socket %d\n", new_socket);
                    
                    // 设置新socket为非阻塞
                    setnonblocking(new_socket);
                    
                    // 注册新socket到epoll
                    struct epoll_event client_ev;
                    
                    // 根据选择的模式设置事件
                    if (trigger_mode == MODE_ET) {
                        client_ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                    } else {
                        client_ev.events = EPOLLIN | EPOLLRDHUP;
                    }
                    
                    client_ev.data.fd = new_socket;
                    
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &client_ev) == -1) {
                        perror("epoll_ctl: new_socket");
                        close(new_socket);
                    }
                }
                
                // 检查accept是否出错
                if (new_socket == -1) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("accept");
                    }
                }
            } 
            // 客户端断开连接
            else if (events[i].events & EPOLLRDHUP) {
                int client_fd = events[i].data.fd;
                printf("Client %d disconnected (EPOLLRDHUP)\n", client_fd);
                close(client_fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            }
            // 客户端socket有数据可读
            else if (events[i].events & EPOLLIN) {
                int client_fd = events[i].data.fd;
                
                // 处理客户端数据（根据选择的模式）
                handle_client_data(client_fd, trigger_mode);
                
                // 在ET模式下，如果读取过程中出错或连接关闭，需要移除socket
                if (trigger_mode == MODE_ET) {
                    // 检查socket是否仍然有效
                    char test_buf[1];
                    if (recv(client_fd, test_buf, sizeof(test_buf), MSG_PEEK | MSG_DONTWAIT) == 0) {
                        printf("Client %d disconnected (after ET read)\n", client_fd);
                        close(client_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                    }
                }
            }
            // 错误处理
            else if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                int client_fd = events[i].data.fd;
                printf("Error on client %d, closing connection\n", client_fd);
                close(client_fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
            }
        }
    }
    
    close(server_fd);
    return 0;
}