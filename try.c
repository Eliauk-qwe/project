#include<stdio.h>
#include <linux/limits.h> //包含PATH_MAX =260
#include<string.h>
#include<sys/stat.h> //提供 stat函数
#include<sys/types.h>
#include<stdlib.h>
#include<pwd.h>  //找uid
#include<grp.h>  //找gid
#include<time.h>  //转换时间
#include<unistd.h>
#include<dirent.h>
#include<errno.h>
#define PARAM_NONE 0   //通过二进制 | 来标记flag
#define PARAM_a 1
#define PARAM_l 2
#define PARAM_R 4
#define PARAM_t 8
#define PARAM_r 16
#define PARAM_i 32
#define PARAM_s 64
#define MAXROWLEN  155 
int h=0,h_max=2; //h_max一行最多多少个文件名
int flag1=0;  
int g_maxlen;    //最长文件名长度
int g_leave_len = MAXROWLEN;  //本行剩下多少空间
int flag=1;
void my_error(const char * err_string,int line);    
void  display_single(char *name ,int color);
void ls_l(struct stat buf,char *name,int color);
void ls_i(char *name,int color);
void ls_s(char * name,int color);
int ls_R(char *path,int flag);
void display_dir(int flag,char*path);
void display_file(int flag,char *filename);
int cmp(const void *_a,const void *_b);
 


 
int cmp(const void *_a,const void *_b)
{
    char *a=(char*)_a;
    char *b=(char*)_b;
    return strcmp(a,b)>0;
}
 

int ls_R(char*name,int flag)
 
{   
DIR * dir;
struct dirent  *ptr;
int   i,count = 0;
struct stat buf;
char name_dir[1000];
if(chdir(name)<0)                              //将输入的目录改为当前目录，下面操作的前提
{ 
  if(errno==13)
  {  errno=0;
   printf("%s/%s :",name_dir,name);
    printf("no perssion\n"); 
   
  }
  
  else 
  my_error("chdir",__LINE__);
   
 
    return -1;
   
}
if(getcwd(name_dir,1000)<0){
  
  my_error("getcwd",__LINE__);  
  return 0;                //获取当前目录的绝对路径（重要，下面的打开目录的操作需要这个路径，否则需要手动添加）
}
 printf("%s:\n",name_dir);
 
 dir = opendir(name_dir);     //用新获得的路径打开目录
if(dir==NULL){
  if(errno==13)
  {  errno=0;
    printf("Permission denied 1\n");
    
  }
  
  else 
  {
    my_error("opendir",__LINE__);
    
  }
     
    return 0;
}
while((ptr = readdir(dir))!=NULL){
  if(g_maxlen<strlen(ptr->d_name))
         g_maxlen = strlen(ptr->d_name);
    count++;
}
 
closedir(dir);
 
//动态数组
  char**filenames =(char**)malloc((count)*sizeof(char*));    //要进行初始化 
  memset(filenames,0,sizeof(char*)*(count));
 
for(i=0;i<count;i++){
 
  filenames[i]=(char*)malloc(1024*sizeof(char));
  memset(filenames[i],0,sizeof(char)*1024);
}
 
 
int j,len=strlen(name_dir),h=0;
 
dir = opendir(name_dir);
for(i=0;i<count;i++){
    ptr = readdir(dir);
    if(ptr == NULL){
        if(errno==13)
  {     errno=0;
    printf("perssioned error\n");
   
  }   
      else 
      my_error("readdir",__LINE__);
 
      continue;
    }
   
    strcat(filenames[h++],ptr->d_name);    //这里要注意用之前的初始化
}
for(i=0;i<h;i++)
   {
    
   display_file(flag,filenames[i]);
   }
  printf("\n");
 
     
      for(i=0;i<h;i++){
            
          if(lstat(filenames[i],&buf)==-1)
          {   //printf("%s/%s\n",name_dir,filenames[i]);
              if(errno==13)
              { printf("%s/%s\n",name_dir,filenames[i]);
                errno=0;
                printf("perrsion denied\n");
                
              }
               free(filenames[i]);
               //my_error("lstat",__LINE__);
                 
             continue;
             
          }
          else
          {
            
          if(strcmp(filenames[i],"..")==0)
          { 
           free(filenames[i]);
          continue;
          }
        else if(strcmp(filenames[i],".")==0)
          { free(filenames[i]);
          continue;
          }
           else if(S_ISDIR(buf.st_mode)){
            if(ls_R(filenames[i],flag)!=-1)
            chdir("../");
            free(filenames[i]);
             
          }
          else if(!S_ISDIR(buf.st_mode))
           {
             continue;
             free(filenames[i]);
           }
           
 
           }
      }
         
               
                  //处理完一个目录后返回上一层
        
        
    free(filenames);
    closedir(dir);
     return 1;          //在函数开始时打开，结束时关闭
    }
 
 
void time_quicksort(long filetime[],char filename[256][PATH_MAX+1],int begin,int end)
{  char tmpname[PATH_MAX+1];
   if(begin>=end)
   return ;
    int i=begin;
    int j=end;
    long t;
    long tmp=filetime[begin];
    while(i!=j)
    {
      while(filetime[j]<=tmp&&j>i)
      j--;
      while(filetime[i]>=tmp&&j>i)
      i++;
      if(j>i)
      { //换时间
        t=filetime[i];
        filetime[i]=filetime[j];
        filetime[j]=t;
        //换文件名
        strcpy(tmpname,filename[i]);
        strcpy(filename[i],filename[j]);
        strcpy(filename[j],tmpname);
 
      }
 
 
    }
    filetime[begin]=filetime[i];
    filetime[i]=tmp;
     strcpy(tmpname,filename[i]);
     strcpy(filename[i],filename[begin]);
     strcpy(filename[begin],tmpname);
 
    time_quicksort(filetime,filename,0,i-1);
     time_quicksort(filetime,filename,i+1,end);
}
 
void display_dir(int flag,char*path)

{
    DIR *dir;
    struct dirent * ptr;
    int cnt=0;  //计算文件个数
    long filetime[256];
    char tmpfilename[PATH_MAX+1];
    struct stat buf;
    dir=opendir(path);
    if(dir==NULL)
    {  
        my_error("opendir",__LINE__);
        return;
    }
    while((ptr=readdir(dir))!=NULL)
     {
         int hanzi=0;
         int nohanzi=0;
         for(int i=0;i<strlen(ptr->d_name);i++)
          {    
              if(ptr->d_name[i]<0)
              hanzi++;
              else
              nohanzi++;
          }
        int len=hanzi*2+nohanzi;
        if(g_maxlen<len)
        g_maxlen=len;
 
        cnt++;
     }
     closedir(dir);
     
     char **filename=(char**)malloc(sizeof(char *)*(cnt+1));
     for(int i=0;i<cnt+1;i++)
      filename[i]=(char*)malloc(sizeof(char)*100);
      dir=opendir(path);
     for(int i=0;i<cnt;i++)
     {
         if((ptr=readdir(dir))==NULL)
          {
              my_error("readdir",__LINE__);
          }
 
         strncpy(filename[i],path,strlen(path));
         filename[i][strlen(path)]='\0';
         strcat(filename[i],ptr->d_name);
         filename[i][strlen(path)+strlen(ptr->d_name)]='\0';
       //printf("%d %s\n",i,filename[i]);
         
 
     }
      closedir(dir);
       if(flag&PARAM_t)
        {    
             flag-=PARAM_t;
             struct stat buf;
             for(int i=0;i<cnt;i++)
              {
                  if(lstat(filename[i],&buf)==-1)
                   {
                       my_error("lstat",__LINE__);
                   }
                   filetime[i]=buf.st_mtime;
              }
 
              for(int i=0;i<cnt;i++)
                   for(int j=i;j<cnt;j++)
                     if(filetime[j]>filetime[i])
                      {
                          long int t;
                          t=filetime[i];
                          filetime[i]=filetime[j];
                          filetime[j]=t;
                     }
                     //time_quicksort(filetime,filename,0,cnt-1);
            
        }
        else   //用文件名首字母排序
         {
              for(int i=0;i<cnt;i++)
               {
                 for(int j=i+1;j<cnt;j++)
                  {
                    if(strcmp(filename[i],filename[j])>0)
                     {
                       char tmp[1024];
                        strcpy(tmp,filename[i]);
                        strcpy(filename[i],filename[j]);
                        strcpy(filename[j],tmp);
                     }
                  }
               }          
 
         }
           //for(int i=0;i<cnt;i++)
            //printf("%d %s\n",i,filename[i]);
          int total=0;
         //计算总量??
          if(flag&PARAM_a)   //包括隐藏文件
           {
               for(int i=0;i<cnt;i++)
                {
                    if(lstat(filename[i],&buf)==-1)
                     {
                         my_error("lstat",__LINE__);
                     }
                    total=total+buf.st_blocks/2; //？？？
                }
           }
          else
           {
                for(int i=0;i<cnt;i++)
                {   
                    if(lstat(filename[i],&buf)==-1)
                     {   
                         my_error("lstat",__LINE__);
                     }
                     if(filename[i][2]!='.')
                    total=total+buf.st_blocks/2; //？？？
                }
           }
 
           if((flag&PARAM_l||flag&PARAM_s))
            {
               printf("总用量 %d\n",total);
            }
 
 
            if(flag&PARAM_r)
              {    
                  if(flag&PARAM_R) 
                    {       flag-=PARAM_R;
                            ls_R(path,flag);
                    }
                    else
                     {    flag-=PARAM_r;
                         for(int i=cnt-1;i>=0;i--)
                           {  
                           display_file(flag,filename[i]);
                           }
                     }
              }
            else
             {
                     if(flag&PARAM_R)
                      {   flag-=PARAM_R;
                          
                          ls_R(path,flag);
                          
                      }
                      else
                       {   
                           for(int i=0;i<cnt;i++)
                            {   
                            display_file(flag,filename[i]);
                            }
                       }
 
             }
          for(int i=0;i<cnt+1;i++)
    {
      free(filename[i]);
    }
    free(filename);
}

