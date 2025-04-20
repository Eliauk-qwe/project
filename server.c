#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include "proto.h"

#define IPSIZE  40
#define BUFSIZE  1024

void server_job_send(int fd);

int main(){
    int server_fd,new_server_fd;
    struct sockaddr_in server_addr,cilent_addr;
    char cilent_ip[IPSIZE];

    //创建socket
    //socket() 建立一个用于交流的端点并且返回一个描述符。
    server_fd= socket(AF_INET,SOCK_STREAM,0/*IPPROTO_TCP*/);
    if(server_fd<0){
        perror("socket() \n");
        exit(1);
    }


     
    //初始化服务器地址结构体
    //
    //
    /*
         struct sockaddr_in {
            sa_family_t    sin_family; // 地址族（AF_INET）
            in_port_t      sin_port;   // 端口号（用 htons() 转换）
            struct in_addr sin_addr;   // IP 地址（用 inet_pton() 转换）
            char           sin_zero[8]; // 填充用，无意义
        };    

        struct in_addr {
            in_addr_t s_addr; // IPv4 地址（网络字节序）
        };

    */
    memset(&server_addr,0,sizeof(server_addr));    //如果你不清零，结构体里的这些未初始化的字段会包含随机值（“脏数据”），可能会导致程序运行时行为不一致，甚至出现错误。
    server_addr.sin_family=AF_INET;                // 设置地址族
    server_addr.sin_port=htons(atoi(SERVERPORT));  // 设置端口（网络字节序）
    inet_pton(AF_INET,"0.0.0.0",&server_addr.sin_addr.s_addr);    // 设置 IP
    





    ///绑定
    //
    //
    //bind  为套接字  sockfd  指定本地地址 my_addr.  my_addr 的长度为 addrlen
    // (字节).传统的叫法是给一个套接字分配一个名字.  
    //当使用 socket(2),  函数创建一个套接字时,它存在于一个地址空间(地址族), 但还没有给它分配一个名字

    if(bind(server_fd,&server_addr,sizeof(server_addr))<0){
        perror("bind()");
        exit(1);
    }




    // 监听
    //
    //
    //1.listen() 用于把一个 socket 设置成 被动监听状态，让它可以接受来自客户端的连接。
    //2.参数     sockfd：socket() 创建并绑定过地址的 socket 文件描述符。
    //          backlog：等待连接队列的最大长度（排队的“访客”数量）。
    //                   注意：这并不是同时能服务多少个客户端，而是正在等待 accept 的客户端连接数上限。 

    if(listen(server_fd,10)<0){
        perror("listen");
        exit(1);
    }

    /*
         两个重要的队列
         当你在服务器端调用 listen(sockfd, backlog)，操作系统会维护 两个队列：

        1️⃣ 半连接队列（SYN Queue）
        存放的是还没有完成三次握手的连接请求。

        客户端发来 SYN 后，内核把它放进这个队列。

        一旦三次握手完成，连接就转移到全连接队列。

        2️⃣ 全连接队列（Accept Queue）
        存放的是已经完成三次握手，但你还没 accept 的连接。

        accept() 会从这个队列中取出一个连接，并返回一个新的 socket。


    */

   /*
       1. 服务端调用 socket() -> bind() -> listen()
       2. 客户端连接发起三次握手
       3. 内核将完成三次握手的连接放入 Accept Queue
       4. 服务端调用 accept() 从 Accept Queue 取出一个连接，返回新的 socket fd
       5. 用这个新 socket fd 进行 send/recv 通信

   */
    
    socklen_t cilent_len=sizeof(cilent_addr);

    while(1){
    new_server_fd=accept(server_fd,(struct sockaddr *)&cilent_addr,&cilent_len);
    if(new_server_fd<0){
        perror("accept");
        exit(1);
    }

    inet_ntop(AF_INET,&cilent_addr.sin_addr.s_addr,cilent_ip,IPSIZE);
    printf("客户端 IP: %s ，端口: %d\n", cilent_ip, ntohs(cilent_addr.sin_port));



    server_job_send(new_server_fd);

    close(new_server_fd);

    }

    close(server_fd);
    exit(0);
}




void server_job_send(int fd){

    char buf[BUFSIZE];
    int len, recv_len;

    // 先接收客户端数据
    recv_len = recv(fd, buf, BUFSIZE - 1, 0);
    if (recv_len < 0) {
        perror("recv");
        exit(1);
    } else if (recv_len == 0) {
        printf("客户端关闭连接\n");
        return;
    }
    buf[recv_len] = '\0';
    printf("收到客户端消息: %s\n", buf);

    len=sprintf(buf,FMT_STAMP,(long long)time(NULL));
    if(send(fd,buf,len,0)<0){
        perror("send");
        exit(1);
    }

}