#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*#include <unistd.h>

unsigned int alarm(unsigned int seconds);

返回值,返回0或剩余的秒数,无失败;*/

int main(void) {
    alarm(10);
    alarm(5);
    alarm(10);
    /*
       每个进程都有且只有唯一的一个定时器，
       所以多个alarm函数共同调用时，后面设置的时钟会覆盖掉前面的时钟。
    */
    
    while(1);
    exit(0);
}
