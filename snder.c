#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>  // 添加：解决struct ip_mreqn定义问题
#include <net/if.h>   

#include "proto.h"

int main(int argc,char **argv){

    int  sd;
    struct msg_st sbuf;
    struct sockaddr_in raddr;
    struct ip_mreqn mreq;
    
    

    sd=socket(AF_INET,SOCK_DGRAM,0);
    if(sd<0){
        perror("socket()");
        exit(1);
    }

    inet_pton(AF_INET, MTGOURP, &mreq.imr_multiaddr); // 多播地址
    inet_pton(AF_INET, "0.0.0.0", &mreq.imr_address); // 自己的地址
    mreq.imr_ifindex = if_nametoindex("wlp2s0"); // 网络设备的索引号


    if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, &mreq, sizeof(mreq)) < 0) {
        perror("setsockopt()");
        exit(1);
    }


    //bind();
    memset(&sbuf,'\0',sizeof(sbuf));
    strcpy(sbuf.name,"wly");
    sbuf.math=htonl(rand()%100);
    sbuf.chinese=htonl(rand()%100);


    raddr.sin_family=AF_INET;
    raddr.sin_port=htons(atoi(REVPORT));
    
    inet_pton(AF_INET,MTGOURP,&raddr.sin_addr);


    if(sendto(sd,&sbuf,sizeof(sbuf),0,(const struct sockaddr*)&raddr,sizeof(raddr))<0){
        perror("sendto()");
        exit(1);
    }


    puts("OK!");


    close(sd);



    exit(0);
}