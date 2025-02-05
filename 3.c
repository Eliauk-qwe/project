int search(int* arr, int arrSize, int target)
{
int i=0;
int flag=0;
for(i=0;i<arrSize;++i)
{
if(arr[i]==target)
{
flag=1;
break;
}
}
if(!flag)
{
return -1;
}
return i;
}

