#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#include "proto.h"

int main(int argc,char **argv){

    int  sd;
    struct msg_st sbuf;
    struct sockaddr_in raddr;

    if(argc<2){
        fprintf(stderr,"usage");
        exit(1);
    }
    

    sd=socket(AF_INET,SOCK_DGRAM,0);
    if(sd<0){
        perror("socket()");
        exit(1);
    }


    //bind();
    strcpy(sbuf.name,"wly");
    sbuf.math=htonl(rand()%100);
    sbuf.chinese=htonl(rand()%100);


    raddr.sin_family=AF_INET;
    raddr.sin_port=htons(atoi(REVPORT));
    
    inet_pton(AF_INET,argv[1],&raddr.sin_addr);


    if(sendto(sd,&sbuf,sizeof(sbuf),0,(const struct sockaddr*)&raddr,sizeof(raddr))<0){
        perror("sendto()");
        exit(1);
    }


    puts("OK!");


    close(sd);



    exit(0);
}