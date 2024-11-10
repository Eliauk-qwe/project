#include <stdio.h>
#include <math.h>

int main()
{
    char word[100];
    scanf("%s",word);
    int len=strlen(word);
    for(int i=len;i>=0;i--)
    printf("%c",word[i]);
}
