#include <stdio.h>
int main()
{
    int count=0;
    int flag;
    char ch;
    while ((ch=getchar())!='#')
    {
        switch (ch)
        {
        case  'e':
            flag=1;
            break;

        case 'i':
         if(flag==1){
            count++;
            flag=0;
         }
         break;
        
        default:
            flag=0;
            break;
        }
    }
    printf("\n%d\n",count);
    return 0;
    
}