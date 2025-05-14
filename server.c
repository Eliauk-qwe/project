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
#include <errno.h>

#define CONTROL_PORT 2100
#define BUFFER_SIZE 1024

typedef struct {
    int control_socket;
    struct sockaddr_in client_addr;
} client_info_t;

typedef struct {
    int data_socket;
    int control_socket; // 新增控制套接字字段
    char command[16];
    char filename[256];
} data_conn_t;

void send_file_list(int data_socket) {
    DIR *dir;
    struct dirent *entry;
    char buffer[BUFFER_SIZE];

    dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        snprintf(buffer, sizeof(buffer), "%s\r\n", entry->d_name);
        send(data_socket, buffer, strlen(buffer), 0);
    }

    closedir(dir);
}

void receive_file(int data_socket, const char *filename, int control_sock) {
    char buffer[BUFFER_SIZE];
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        send(control_sock, "550 File operation failed\r\n", 25, 0);
        perror("open for write");
        return;
    }

    ssize_t n;
    while ((n = recv(data_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        write(fd, buffer, n);
    }
    close(fd);
}

void send_file(int data_socket, const char *filename, int control_sock) {
    char buffer[BUFFER_SIZE];
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        send(control_sock, "550 File not found\r\n", 20, 0);
        perror("open for read");
        return;
    }

    ssize_t n;
    while ((n = read(fd, buffer, BUFFER_SIZE)) > 0) {
        send(data_socket, buffer, n, 0);
    }
    close(fd);
}

void *data_thread_func(void *arg) {
    data_conn_t *data = (data_conn_t *)arg;
    int sock = data->data_socket;

    if (strcmp(data->command, "LIST") == 0) {
        send_file_list(sock);
    } else if (strcmp(data->command, "STOR") == 0) {
        receive_file(sock, data->filename, data->control_socket);
    } else if (strcmp(data->command, "RETR") == 0) {
        send_file(sock, data->filename, data->control_socket);
    }

    close(sock);
    free(data);
    return NULL;
}

void *control_thread_func(void *arg) {
    client_info_t *client = (client_info_t *)arg;
    int control_sock = client->control_socket;
    free(client);

    send(control_sock, "220 FTP Server Ready\r\n", 23, 0);

    int pasv_socket = -1;
    char command[BUFFER_SIZE], cmd[16], arg1[256];

    while (1) {
        memset(command, 0, sizeof(command));
        if (recv(control_sock, command, sizeof(command), 0) <= 0) break;

        sscanf(command, "%15s %255s", cmd, arg1);
        printf("CMD: %s ARG: %s\n", cmd, arg1);

        if (strcmp(cmd, "PASV") == 0) {
            pasv_socket = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in pasv_addr;
            socklen_t len = sizeof(pasv_addr);
            
            memset(&pasv_addr, 0, sizeof(pasv_addr));
            pasv_addr.sin_family = AF_INET;
            pasv_addr.sin_addr.s_addr = INADDR_ANY;
            pasv_addr.sin_port = 0;
            bind(pasv_socket, (struct sockaddr*)&pasv_addr, sizeof(pasv_addr));
            listen(pasv_socket, 1);
            
            getsockname(pasv_socket, (struct sockaddr*)&pasv_addr, &len);
            int data_port = ntohs(pasv_addr.sin_port);
            int p1 = data_port / 256, p2 = data_port % 256;

            // 动态获取本地IP
            struct sockaddr_in local_addr;
            socklen_t local_len = sizeof(local_addr);
            getsockname(control_sock, (struct sockaddr*)&local_addr, &local_len);
            char *ip_str = inet_ntoa(local_addr.sin_addr);
            int h1, h2, h3, h4;
            sscanf(ip_str, "%d.%d.%d.%d", &h1, &h2, &h3, &h4);

            char response[128];
            sprintf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", h1, h2, h3, h4, p1, p2);
            send(control_sock, response, strlen(response), 0);

        } else if (strcmp(cmd, "LIST") == 0 || strcmp(cmd, "RETR") == 0 || strcmp(cmd, "STOR") == 0) {
            if (pasv_socket == -1) {
                send(control_sock, "425 No data connection\r\n", 23, 0);
                continue;
            }

            struct sockaddr_in client_data_addr;
            socklen_t client_len = sizeof(client_data_addr);
            int data_sock = accept(pasv_socket, (struct sockaddr*)&client_data_addr, &client_len);

            data_conn_t *conn = malloc(sizeof(data_conn_t));
            conn->data_socket = data_sock;
            conn->control_socket = control_sock;
            strncpy(conn->command, cmd, sizeof(conn->command));
            strncpy(conn->filename, arg1, sizeof(conn->filename));

            pthread_t tid;
            pthread_create(&tid, NULL, data_thread_func, conn);
            pthread_detach(tid);

            close(pasv_socket);
            pasv_socket = -1;
        } else if (strcmp(cmd, "QUIT") == 0) {
            send(control_sock, "221 Goodbye\r\n", 13, 0);
            break;
        } else {
            send(control_sock, "500 Unknown command\r\n", 24, 0);
        }
    }

    close(control_sock);
    return NULL;
}

int main() {
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(CONTROL_PORT);

    bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(listen_sock, 5);

    printf("FTP Server listening on port %d...\n", CONTROL_PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &addrlen);

        client_info_t *info = malloc(sizeof(client_info_t));
        info->control_socket = client_sock;
        info->client_addr = client_addr;

        pthread_t tid;
        pthread_create(&tid, NULL, control_thread_func, info);
        pthread_detach(tid);
    }

    close(listen_sock);
    return 0;
}