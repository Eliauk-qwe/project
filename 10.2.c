#include <stdio.h>
void copy_arr(double t[],double s[],int n);
void copy_ptr(double *t[],double *s[],int n);
void copy_ptrs(double *t[],double *s[],int n);




int main()
{
    double s[5]={1.1,2.2,3.3,4.4,5.5};
    double t1[5];
    double t2[5];
    double t3[5];
    void copy_arr(t1,s,5);
    void copy_ptr(t2,s,5);
    void copy_ptrs(t3,s,s+5);
    return 0;
    





    
}
void copy_arr(double t[],double s[],int n)
{
    for(int i=0;i<n;i++)
        t[i]=s[i];
}
void copy_ptr(double *t[],double *s[],int n)
{
    for(int i=0;i<n;i++)
        *(t+i)=*(s+i);
}
void copy_ptrs(double *t,double *s_first,double *s_last)
{

    for(int i=0;(s_last-s_first)>i;i++)
    for(int i=0;(s_last-s_first)>0;i++,s_first++)
    *(t+1)=*(s_first+i);

}

