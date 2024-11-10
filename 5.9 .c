#include <stdio.h>
int Temperatures(const double n);
int main()
{
    double n;
    while (scanf("%lf",&n)!=0)
    {

       Temperatures( n);
        
        
        
    }
    return 0;
    
}

int Temperatures(const double n)
{
    double T,K;
    T=5.0/9.0*(n-32.0);
    K=n+273.16;
    printf("摄氏温度为%lf\n",T);
    printf("开氏温度为%lf\n",K);
    return 0;



}