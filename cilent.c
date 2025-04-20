#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>



#include "proto.h"


int main(int argc,char** argv){
    int cilent_fd,fp;
    struct sockaddr_in  server_addr;
    long long stamp;
    char buffer[1024];

    if(argc<2){
        fprintf(stderr,"usage...");
        exit(1);
    }

    cilent_fd=socket(AF_INET,SOCK_STREAM,0);
    if(cilent_fd<0){
        perror("socket()");
        exit(1);

    }
    
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(atoi(SERVERPORT));
    inet_pton(AF_INET,argv[1],&server_addr.sin_addr.s_addr);

    if(connect(cilent_fd,&server_addr,sizeof(server_addr))<0){
        perror("connect");
        exit(1);
    }

    /*fp=fdopen(cilent_fd,"r+");
    if(fp == NULL) {
        perror("dfopen()");
        exit(1);
    }

    if(fscanf(fp,FMT_STAMP,&stamp)<1){
         fprintf(stderr, "Bad format!\n");
    }else {
        fprintf(stdout, "stamp = %lld\n", stamp);
    }


    // 按照标准io的方式关闭fp
    fclose(fp);
    exit(0);*/


    // 发送和接收消息
   /* send(cilent_fd, "你好，服务器！", 18, 0);
    recv(cilent_fd, buffer, sizeof(buffer) - 1, 0);
    printf("收到服务器消息：%s\n", buffer);*/


    // 发送消息到服务器
    char *message = "你好，服务器！";
    if (send(cilent_fd, message, strlen(message), 0) < 0) {  // 修正长度计算
        perror("send");
        close(cilent_fd);
        exit(1);
    }

    // 接收服务器响应
    int recv_len = recv(cilent_fd, buffer, sizeof(buffer) - 1, 0);
    if (recv_len < 0) {
        perror("recv");
    } else if (recv_len == 0) {
        printf("服务器关闭连接\n");
    } else {
        buffer[recv_len] = '\0';
        printf("收到服务器响应: %s\n", buffer);
    }

    // 关闭连接
    close(cilent_fd);
    return 0;





}