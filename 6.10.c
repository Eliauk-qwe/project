#include <stdio.h>
int main()
{
    int m,n;
    int sum=0;
    scanf("%d %d",&m,&n);
    while (m<n)
    {
        for(int i=m;i<=n;i++)
        {
            int n=i*i;
            sum=sum+n;
        }
        printf("%d\n",sum);
        sum=0;
        scanf("%d %d",&m,&n);
        

    }
    
}