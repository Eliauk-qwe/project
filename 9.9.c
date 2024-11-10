#include <stdio.h>
double power(double n,int p);
int main()
{
    double x,xpow;
    int n;
    while (scanf("%lf %d",&x,&n)==2)
    {
        xpow=power(x,n);
        printf("%.3g to the power %d is %.5g\n",x,n,xpow);
    }
    printf("bye");
    
}

double power(double n,int p){
    double pow=1;
    int i;
    if(n==0&&p==0){
        printf("%g %c",n,p);
        return 1;
    }
    if(n==0) return 0;
    if(p==0) return 1;
    if(p>0){
        return n*power(n,p-1);
    }else{
        return power(n,p+1)/n;
    }


}
