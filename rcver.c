#include <stdio.h>
#include <stdlib.h>
#include "proto.h"
#include <sys/socket.h>
#include <sys/types.h>
//#include <netinet/in.h>
#include <arpa/inet.h>

#define IPSTRSIZE 40

int main(){
    int sd;
    struct sockaddr_in  laddr,raddr;
    struct msg_st rbuf;
    socklen_t raddr_len;
    char  ipstr[IPSTRSIZE];


    sd=socket(AF_INET,SOCK_DGRAM,0);
    if(sd<0){
        perror("socket()");
        exit(1);
    }

    laddr.sin_family=AF_INET;
    laddr.sin_port=htons(atoi(REVPORT));

    //10 to 2
    inet_pton(AF_INET,"0.0.0.0",&laddr.sin_addr);


    if(bind(sd,(struct sockaddr*)&laddr,sizeof(laddr))<0){
        perror("bind()");
        exit(1);

    }

    //!!!!!1!1!11！初始化
    raddr_len=sizeof(raddr);

    while (1)
    {
        recvfrom(sd,&rbuf,sizeof(rbuf),0,(void*)&raddr,raddr_len);

        //2 to 10
        inet_ntop(AF_INET,&raddr.sin_addr,ipstr,IPSTRSIZE);
        printf("---message from %s :%d---\n",ipstr,ntohs(raddr.sin_port));
        printf("name=%s\n",rbuf.name);
        printf("math=%d\n",ntohl(rbuf.math));
        printf("chinese=%d\n",ntohl(rbuf.chinese));

    }
    
    

    close(sd);

    exit(0);
    
   
}