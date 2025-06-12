#include <iostream>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "../ThreadPool.hpp"
#include <fcntl.h>


using namespace std;

#define LISTEN_NUM 150
#define MAX_EVENTS 1024

// 设置文件描述符为非阻塞模式
void setnoblock(int fd){
    int flag =fcntl(fd,F_GETFL);
    if(flag<0){
        cerr << "fcntl" <<endl;
        exit(EXIT_FAILURE);
    }

    if(fcntl(fd,F_SETFL,flag|O_NONBLOCK)  <0){
        cerr << "fcntl" <<endl;
        exit(EXIT_FAILURE);
    }
}



int  main(int argc,char *argv[]){
    //再命令行要输入指定
    if(argc !=3){
        cerr << "Usage : " << argv[0] << "<IP> <PORT>" << endl;
        exit(EXIT_FAILURE);
    }

    
    int server_fd=socket(AF_INET,SOCK_STREAM,0) ;
    if(server_fd<0){
        cerr << "socket err" <<endl;
    }


    //完成S 端均支持在 Web 自行指定 IP:Port
    //设置默认服务端IP（初始化）
    sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    //server_addr.sin_addr.s_addr=htonl(argv[1]);
    if(inet_pton(AF_INET,argv[1],(sockaddr*)&server_addr)  <=0){
        cerr << "Invailed  server IP " << endl;
        close(server_fd);
        exit(EXIT_FAILURE);

    }
    uint32_t port=stoi(argv[2]);
    server_addr.sin_port=htonl(port);

    //允许端口复用
    int opt=1;
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
 
    socklen_t server_len =sizeof(server_addr);
    if(bind(server_fd,(sockaddr*)&server_addr,server_len) <0){
        cerr << "bind " << endl;
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd,LISTEN_NUM) <0){
        cerr << "listen " << endl;
        close(server_fd);
        exit(EXIT_FAILURE);

    }


    //使用epoll
    int epoll_fd=epoll_create1(0);
    if(epoll_fd < 0){
        cerr << "epoll_create1" <<endl;
        exit(EXIT_FAILURE);
    }

    setnoblock(server_fd);
    struct epoll_event ev;
    ev.data.fd=server_fd;
    ev.events = EPOLLIN | EPOLLET;
    //将文件描述符（FD）注册到 epoll 实例中，监控指定事件。
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,server_fd,&ev) < 0 ){
        cerr << "epoll_ctl" <<endl;
        exit(EXIT_FAILURE);
    }
    /*
    struct epoll_event {
        uint32_t     events;  // 事件掩码 (EPOLLIN | EPOLLOUT | ...)
        epoll_data_t data;    // 用户数据
    };

    typedef union epoll_data {
        void    *ptr;   // 自定义数据结构指针
        int      fd;    // 关联的文件描述符
        uint32_t u32;
        uint64_t u64;
    } epoll_data_t;

    */


   //线程池，创建10个线程
   ThreadPool(10);

   cout << "服务器开始工作" << endl;

   //======================================================================
   //心跳检测
   //======================================================================
   
   
   // 存储就绪事件的数组
   struct epoll_event events[MAX_EVENTS];
   while(1){
        // 等待事件发生（-1表示无限等待），返回值为就绪事件数量
        int  count = epoll_wait(epoll_fd,events,MAX_EVENTS,-1); 
        if(count < 0){
            cerr << "epoll_wait" <<endl;
            exit(EXIT_FAILURE);
        }

        cout << "Epoll returned " << count << " events" << endl;

        for(int i=0;i<count;i++){
            int fd = events[i].data.fd;
              
            //i=0,服务端接受客户端的连接  
            if(fd==server_fd){
                sockaddr_in client_addr;
                socklen_t client_len =sizeof(client_addr);
                int client_fd=accept(server_fd,(sockaddr*)&client_addr,&client_len);

                setnoblock(client_fd);
                struct epoll_event client_ev;
                client_ev.data.fd=client_fd;
                client_ev.events=EPOLLIN | EPOLLET;
                if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,client_fd,&client_ev) < 0 ){
                    cerr << "epoll_ctl" <<endl;
                    exit(EXIT_FAILURE);
                }

            }
        }

   }






     
    
 
    


}