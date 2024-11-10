#include <stdio.h>
#define SIZE 40;
char * read(char*s,int n)
{
    int i=0;
    do{
        s[i]=getchar();
        if(s[i]=='\n'||s[i]=='\t'||s[i]==' ')
        break;

    }while(s[i]!=EOF&&++i<n);
    return s;
}







int main()
{
   char test[SIZE];
   read(test,SIZE);
   puts(test);
   return 0;
}
