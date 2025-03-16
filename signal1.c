#include <stdio.h>
#include <stdlib.h>
#include <signal.h>


int main(){
    //signal(SIGINT,SIG_IGN);
    

    for(int i=0;i<10;i++){
        write(1,"*",1);
        sleep(1);
    }
    printf("\n");

    exit(0);
}