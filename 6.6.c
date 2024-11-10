#include <stdio.h>
int main()
{
    int m,n;
    scanf("%d %d",&m,&n);
    for(int i=m;i<=n;i++)
    {
        printf("%d\t %d*%d=%d\t\t %d*%d*%d=%d\n",i,i,i,i*i,i,i,i,i*i*i);
    }
}