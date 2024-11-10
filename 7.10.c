#include <stdio.h>
int main()
{
    int n;
    do{
        scanf("%d",&n);
        if(n<2){
            if(n==0)  break;
            printf("%d is out of range,retry.\n",n);
            continue;

        }
        for (int i = n; i >1; i--){
            int is_prime=1;
            for(int j=2;j<i/2;j++){
                if(i%j==0){
                    is_prime=0;
                    break;
                }
            }
            if(is_prime==1)
               printf ("%d,",i);

        }
        printf("\n");
        

    }while (n!=0);
    return 0;
    
    
}
