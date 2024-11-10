#include <stdio.h>
void ordering(double *x,double*y,double*z);

int main()
{
    double n1,n2,n3;
    scanf("%lf %lf %lf");
    ordering(&n1,&n2,&n3);
    printf("%g %g %g",n1,n2,n3);
}
void ordering(double *x,double*y,double*z)
{
    double t;
    if(*x>*y){
        t=*x;
        *x=*y;
        *y=t;
    }
    if(*x>*z){
        t=*x;
        *x=*z;
        *z=t;
    }
    if(*y>*z){
        t=*y;
        *y=*z;
        *z=t;
    }
    

    
}