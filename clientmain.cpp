#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include "client.hpp"



using namespace std;
StickyPacket socket_fd("receive");

int main(int argc,char *argv[]){
    if(argc !=3){
        cerr << "Usage : " << argv[0] << " <IP> <PORT>" << endl;
        exit(EXIT_FAILURE);
    }

    /*int fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd<0){
        perror("socket failed!\n");
        close(fd);
        exit(EXIT_FAILURE);
    }*/

    int fd=socket_fd.getfd();
    sockaddr_in client_addr;
    memset(&client_addr,0,sizeof(client_addr));
    client_addr.sin_family=AF_INET;
    uint16_t port=stoi(argv[2]);
    client_addr.sin_port=ntohs(port);
    if((AF_INET,argv[1],(sockaddr*)&client_addr.sin_addr) <0){
        perror("inet_pton failed!\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    socklen_t len=sizeof(client_addr);
    if(connect(fd,(sockaddr*)&client_addr,len)<0){
        perror("connect failed!\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // 命令循环
    while (1) {
        cout<<"---------欢迎来到聊天室---------"<<endl;
        printf("选项:\n1.注册新账户\n2.登录账户\n3.找回密码\n4.退出\n输入选择: ");
        string  opt;
        getline(cin,opt);

        int res;
        

        switch (stoi(opt)) {
            case 1: {
                sign_up();
                break;
            }
            case 2: {
                res=log_in();
                break;
            }
            case 3: {
                pass_find();
                break;
            }
            case 4: {

                close(fd);
                printf("连接已关闭\n");
                printf("感谢使用聊天室，再见！\n");
                exit(EXIT_SUCCESS);
            }
            default:
                printf("无效选项,请输入1-4\n");
        }

    }

    if(res==1)  user_menu();


}