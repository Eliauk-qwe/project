#include <stdio.h>
int main()
{
    char n;
    scanf("%c",&n);
    int len=n-65+1;
    
    for (int i = 1; i <=len; i++)
    {
        for (int l = i; l<len; l++)
        {
            printf(" ");
            
            
        }
        
        char a='A';
        for (int j = 1; j <=i; j++,a++)
        {
            printf("%c",a);
            
        
        }
        a=a-2;//?
        for (int k = 1; k <=i-1; k++,a--)
        {
            printf("%c",a);
            
            
        }
        
        printf("\n");
        
    }
    
}