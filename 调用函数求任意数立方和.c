#include <stdio.h>
double  lifang(double n);
int main()

{
    
    double n,sum;
    scanf("%lf",&n);
    sum=lifang(n);
    printf("%f",sum);

    
    return 0;
}

double  lifang(double n)
{
    double sum;
    sum=n*n*n;
    return sum;
    
}