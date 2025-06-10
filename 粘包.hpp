#ifndef _粘包_H_
#define _粘包_H_

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

using namespace std;

class StickyPacket{
private:
    int fd=-1;
    int receive_fd=-1;

public:
    int getfd() {
        return fd;
    }

    int get_receive_fd(){
        return receive_fd;
    }

    StickyPacket(){
        fd=socket(AF_INET,SOCK_STREAM,0);
    }

    StickyPacket(int fd){
        this->fd=fd;
    }

    StickyPacket(string msg){
        if(msg=="receive"){
            fd=socket(AF_INET,SOCK_STREAM,0);
            receive_fd=socket(AF_INET,SOCK_STREAM,0);
        }
    }

    ~StickyPacket()  {}




    ssize_t readn(int fd,void *buffer,size_t size){
        size_t left;
        ssize_t nread;
        size_t count=0;
        char *buf=(char*)buffer;

        
        left=size;

        while(left>0){
            nread=read(fd,buf,left);
            if(nread==0)  break;
            // 处理可恢复错误（被信号中断或非阻塞无数据）
            else if(nread<0){
                if(errno==EINTR || errno==EWOULDBLOCK)  continue;
                else               return -1;
            }

            left=left-nread;
            buf=buf+nread;
            count=count+nread;

        }

        return count;
    }

    int mysend(string message){
        int message_len=message.size();
        int head_len=htonl(message_len);

        //消息封装（消息头+消息体）
        char *msg =new char[message_len+4];
        memcpy(msg,&head_len,4);
        memcpy(msg+4,message.data(),message_len);

        const char *buf=msg;
        int left=message_len+4;
        int nsend;
        int count=0;

        while(left>0){
            nsend=send(fd,&buf,left,0);
            if(nsend<0){
                if(errno==EINTR || errno==EWOULDBLOCK) continue;
                else{
                    close(fd);
                    delete [] msg;
                    perror("send failed()!");
                    return -1;
                }
            }
            else if(nsend==0)  break;

            left=left-nsend;
            buf=buf+nsend;
            count=count+nsend;

        }
        close (fd);
        delete [] msg;
        return count;

    }

    //服务端接受函数
    int server_recv(int cfd,char **msg){
        int len=0;
        readn(cfd,(char *)len,4);
        len=ntohl(len);

        //char *msg =new char[message_len+4];
        char *buf=new char[len+1];
        int nread=readn(cfd,buf,len);
        if(nread!=len){
            cout << "数据读取失败" <<endl;
        }else if(nread==0){
            cout <<to_string(cfd) << "断开连接" <<endl;//======================
            close(cfd);
        }
        buf[len]='\0';
        *msg=buf;
        return nread;
        //============================return meg????
    }


    //客户端接受
    string client_recv(){
        // === 第一步：接收4字节消息头（长度信息） ===
        int len=0;
        char *head_buf=new char[4];
        int left=4;
        char *p=(char *)len;

        while(left>0){
            int nrecv=recv(fd,p,left,0);
            if(nrecv == -1){
                close(fd);
                perror("recv failed!\n");
                exit(0);
            }
            else if(nrecv ==0){
                cout << "连接已结束" <<endl;
                close(fd);
                delete [] head_buf;
                return "close";
            }

            p=p+nrecv;
            left=left-nrecv;
        }

        len=ntohl(len);
        delete [] head_buf;

        char *buf =new char[len+1];
        left=len;
        p=buf;

        while(left>0){
            int nrecv=recv(fd,p,left,0);
            if(nrecv == -1){
                close(fd);
                perror("recv failed!\n");
                delete [] buf;
                exit(0);
            }
            else if(nrecv ==0){
                cout << "连接已结束" <<endl;
                close(fd);
                delete [] buf;
                return "close";
            }

            p=p+nrecv;
            left=left-nrecv;

        }

        buf[len]='\0';
        string msg(buf);
        delete [] buf;
        return msg;

    }
};


#endif


