#include <stdio.h>
int main()
{
    int count=0;
    char ch;
    while ((ch=getchar())!=EOF)
    {
        if(count==10){
            printf("\n");
            count=1;
        }
        if(ch>='\040'){
            printf("\'%c\'--%3d",ch,ch);

        }else if(ch=='\n'){
            printf("\\n--\\n\n");
            count=0;
        }else if(ch=='\t'){
            printf("\\t--\\t");
        }else{
            printf("\'%c\'--^%c",ch,(ch+6));
        }

    }
    
}