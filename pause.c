#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    alarm(5);
    while(1)
        pause();
    exit(0);
}

/*
  当调用到pause()时，该进程挂起，此时不再占用CPU，
  5s过后，接收到SIGALRM信号，采取默认动作终止。
*/