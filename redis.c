#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <hiredis/hiredis.h>

#define PORT 6379        // 服务端端口
#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 6379
#define BUFFER_SIZE 1024

// 错误处理宏
#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

// 连接Redis服务器
redisContext* connect_redis() {
    redisContext *c = redisConnect(REDIS_HOST, REDIS_PORT);
    if (c == NULL || c->err) {
        if (c) {
            printf("Redis连接错误: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("无法分配Redis上下文\n");
        }
        exit(EXIT_FAILURE);
    }
    printf("成功连接到Redis\n");
    return c;
}

// 处理客户端命令
void process_command(int client_fd, redisContext *redis_conn) {
    char buffer[BUFFER_SIZE];
    ssize_t n;

    while ((n = read(client_fd, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[n] = '\0';
        printf("收到命令: %s\n", buffer);

        char *cmd = strtok(buffer, " ");
        char *key = strtok(NULL, " ");
        char *value = strtok(NULL, "\r\n");

        redisReply *reply;

        if (cmd == NULL) {
            const char *err = "错误：无效命令\n";
            write(client_fd, err, strlen(err));
            continue;
        }

        if (strcasecmp(cmd, "SET") == 0 && key != NULL && value != NULL) {
            // 执行Redis SET命令
            reply = redisCommand(redis_conn, "SET %s %s", key, value);
            if (reply == NULL) {
                const char *err = "错误:Redis命令失败\n";
                write(client_fd, err, strlen(err));
            } else if (reply->type == REDIS_REPLY_ERROR) {
                write(client_fd, reply->str, reply->len);
                write(client_fd, "\n", 1);
            } else {
                const char *ok = "OK\n";
                write(client_fd, ok, strlen(ok));
            }
            freeReplyObject(reply);
        } 
        else if (strcasecmp(cmd, "GET") == 0 && key != NULL) {
            // 执行Redis GET命令
            reply = redisCommand(redis_conn, "GET %s", key);
            if (reply == NULL) {
                const char *err = "错误:Redis命令失败\n";
                write(client_fd, err, strlen(err));
            } else if (reply->type == REDIS_REPLY_NIL) {
                const char *not_found = "(nil)\n";
                write(client_fd, not_found, strlen(not_found));
            } else if (reply->type == REDIS_REPLY_STRING) {
                write(client_fd, reply->str, reply->len);
                write(client_fd, "\n", 1);
            } else {
                const char *err = "错误：无效响应\n";
                write(client_fd, err, strlen(err));
            }
            freeReplyObject(reply);
        }
        else if (strcasecmp(cmd, "EXIT") == 0) {
            const char *bye = "再见！\n";
            write(client_fd, bye, strlen(bye));
            break;
        }
        else {
            const char *err = "错误：不支持的命令\n";
            write(client_fd, err, strlen(err));
        }
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // 创建TCP套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        handle_error("socket创建失败");
    
    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // 绑定套接字
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        handle_error("绑定失败");
    
    // 开始监听
    if (listen(server_fd, 5) < 0)
        handle_error("监听失败");
    
    printf("服务器运行在端口 %d...\n", PORT);
    
    // 连接Redis
    redisContext *redis_conn = connect_redis();
    
    while (1) {
        // 接受客户端连接
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) < 0)
            handle_error("接受连接失败");
        
        printf("客户端已连接: %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // 处理客户端命令
        process_command(client_fd, redis_conn);
        
        close(client_fd);
        printf("客户端已断开连接\n");
    }
    
    // 清理资源
    redisFree(redis_conn);
    close(server_fd);
    return 0;
}