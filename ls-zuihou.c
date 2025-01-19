#include<stdio.h>
#include <linux/limits.h> //包含PATH_MAX =260
#include<string.h>
#include<sys/stat.h> 
#include<sys/types.h>
#include<stdlib.h>
#include<pwd.h>  
#include<grp.h>  
#include<time.h>  //转换时间
#include<unistd.h>
#include<dirent.h>
#include<errno.h>  

//通过二进制 | 来标记flag
#define PARAM_NONE 0   
#define PARAM_a 1
#define PARAM_l 2
#define PARAM_R 4
#define PARAM_t 8
#define PARAM_r 16
#define PARAM_i 32
#define PARAM_s 64
#define MAXROWLEN  155 

//对齐
int h=0,h_max=2;    //一行中文件个数
int g_maxlen;  //这个目录下最长的文件名
int g_leave_len = MAXROWLEN;  //本行剩余长度


//函数
void display_dir(int flag,char*path);
void err(const char* err_string,int line);
void display_file(int flag,char *path);
void display_only(const char *name,int filecolor);
void ls_l(struct stat file_stat,const char *filepath,int filecolor);


int main(int argc,char *argv[]){
    char path[PATH_MAX+1];
    int flag=PARAM_NONE;
    char param[1000];
    int count=0,number=1;
    struct stat buf;
    

    for(int i=1;i<argc;i++){
        if(argv[i][0]=='-'){
            for(int j=1;j<strlen(argv[i]);j++){
                 param[count++]=argv[i][j];
            }
        }
        number++;
    }

    for(int i=0;i<count;i++){
        if(param[i]=='a'){
            flag|=PARAM_a;
        }
        else  if(param[i]=='l'){
            flag|=PARAM_l;
        }
        else  if(param[i]=='R'){
            flag|=PARAM_R;
        }
        else  if(param[i]=='t'){
            flag|=PARAM_t;
        }
        else  if(param[i]=='r'){
            flag|=PARAM_r;
        }
        else  if(param[i]=='i'){
            flag|=PARAM_i;
        }
        else  if(param[i]=='s'){
            flag|=PARAM_s;
        }
    }

    //说明命令中没有路径，所以默认路径为./当前目录下
    if(number==1){
        strcpy(path,"./");
        path[2]='\0';
        display_dir(flag,path);
        return 0;
    }

    //通过命令行参数来确定制定目录，并判断是目录还是文件
    for(int i=1;i<argc;i++){
        //通过命令行参数来确定制定目录
        if(argv[i][0]=='-')  continue;
        else{
            if(lstat(argv[i],&buf)==-1){
                err(argv[i],__LINE__);
            }
        }

        //判断是目录还是文件
        strcpy(path,argv[i]);

        //是目录
        if(S_ISDIR(buf.st_mode)){
            if(argv[strlen(argv[i])-1]!='/'){
                argv[strlen(argv[i])-1]='/';
                argv[strlen(argv[i])]!='\0';
            }else{
                argv[strlen(argv[i])]!='\0';
            }
            //printf("%s\n",path);
            if(chdir(path)==-1){
                err("chdir",__LINE__);
            }
            display_dir(flag,path);
        }

        //是文件
        else   display_file(flag,path);

        //printf("\n");
    }
    return 0;
}

void err(const char* err_string,int line){
    // 直接使用全局 errno 和标准错误码
    fprintf(stderr,"line:%d ",line);
    perror(err_string);
    if(errno == EACCES) {
        printf("Permission denied.\n");
    } else {
        exit(EXIT_FAILURE);
    } 
    // 注意：通常不建议在 err 函数中重置 errno，因为这可能会干扰调用者的错误处理。
    // 如果确实需要重置，请在调用 err 函数之后由调用者负责。
}

void ls_l(struct stat file_stat,const char *filepath,int filecolor){
    char colorname[NAME_MAX + 30];
    // 打印文件类型
    if(S_ISLNK(file_stat.st_mode))
        printf("l");
    else if(S_ISREG(file_stat.st_mode))
        printf("-");
    else if(S_ISDIR(file_stat.st_mode))
        printf("d");
    else if(S_ISCHR(file_stat.st_mode))
        printf("c");
    else if(S_ISBLK(file_stat.st_mode))
        printf("b");
    else if(S_ISFIFO(file_stat.st_mode))
        printf("f");
    else if(S_ISSOCK(file_stat.st_mode))
        printf("s");

    // 打印文件权限
    printf( (file_stat.st_mode & S_IRUSR) ? "r" : "-");
    printf( (file_stat.st_mode & S_IWUSR) ? "w" : "-");
    printf( (file_stat.st_mode & S_IXUSR) ? "x" : "-");
    printf( (file_stat.st_mode & S_IRGRP) ? "r" : "-");
    printf( (file_stat.st_mode & S_IWGRP) ? "w" : "-");
    printf( (file_stat.st_mode & S_IXGRP) ? "x" : "-");
    printf( (file_stat.st_mode & S_IROTH) ? "r" : "-");
    printf( (file_stat.st_mode & S_IWOTH) ? "w" : "-");
    printf( (file_stat.st_mode & S_IXOTH) ? "x" : "-");

    // 打印文件的硬链接数
    printf(" %ld ", file_stat.st_nlink);

    // 打印文件所有者
    printf("%s ", getpwuid(file_stat.st_uid)->pw_name);

    // 打印文件所属组
    printf("%s ", getgrgid(file_stat.st_gid)->gr_name);

    // 打印文件大小（字节）
    printf("%ld ", file_stat.st_size);

    // 打印文件最后修改时间
    char time_str[100];
    struct tm *tm_info = localtime(&file_stat.st_mtime);
    strftime(time_str,sizeof(time_str),"%m月 %d %H:%M",tm_info);
    printf("%s ", time_str);


    // 打印文件名
    sprintf(colorname,"\033[%dm%s\033[0m",filecolor,filepath);
    printf(" %-s",colorname);
}

void display_file(int flag,char *path){
    char name[PATH_MAX+1];
    char lj[PATH_MAX+1];
    int j=0,filecolor=37;//h=0
    struct stat buf;

    for(int i=0;i<strlen(path);i++){
        if(path[i]=='/'){
            memset(name,"\0",PATH_MAX+1);
            j=0;
            //h=i
        }
        name[j++]=path[i];
    }
    name[j]='\0';

    //判断颜色
    if(latat(path,&buf)==-1){
        err("lstat",__LINE__);
    }
    if(S_ISDIR(buf.st_mode))       filecolor=34;	
    else if(S_ISBLK(buf.st_mode))  filecolor=36;
    else if(filecolor == 37&&((buf.st_mode & S_IXUSR)||(buf.st_mode & S_IXGRP)||(buf.st_mode & S_IXOTH))) filecolor=33;
    
    //判断命令
    if(flag==PARAM_NONE||flag==PARAM_r){
        if(name[0]!='.')    display_only(name,filecolor);
    }
    else if(flag==PARAM_a)  display_single(name,filecolor);
    else if(flag==PARAM_l){    
        if(name[0]!='.')   ls_l(buf,name,filecolor);
    }
    else if(flag==PARAM_i){    
        if(name[0]!='.'){
            printf("%7ld ", buf.st_ino);
            display_none(name, filecolor);
        }   
    }
    else if(flag==PARAM_s){    
        if(name[0]!='.'){
            printf("%3ld ", buf.st_blocks/2);
            display_none(name, filecolor);
        }   
    }
    else if(flag==(PARAM_a+PARAM_l))  ls_l(buf,name,filecolor);
    else if(flag==(PARAM_a+PARAM_i)){
        printf("%7ld ", buf.st_ino);
        display_none(name, filecolor);
    }
    else if(flag==(PARAM_a+PARAM_s)){
        printf("%3ld ", buf.st_blocks/2);
        display_none(name, filecolor);
    }
    else if(flag==(PARAM_s+PARAM_l)){
        if(name[0]!='.'){
            printf("%3ld ", buf.st_blocks/2);
            ls_l(buf,name,filecolor);
        }  
    }
    else if(flag==(PARAM_s+PARAM_i)){
        if(name[0]!='.'){
            printf("%7ld ", buf.st_ino);
            printf("%3ld ", buf.st_blocks/2);
            display_none(name, filecolor);
        }  
    }
    else if(flag==(PARAM_i+PARAM_l)){
        if(name[0]!='.'){
            printf("%7ld ", buf.st_ino);
            ls_l(buf,name,filecolor);
        }  
    }
    else if(flag==(PARAM_s+PARAM_l+PARAM_i)){
        if(name[0]!='.'){
            printf("%7ld ", buf.st_ino);
            printf("%3ld ", buf.st_blocks/2);
            ls_l(buf,name,filecolor);
        }  
    }
    else if(flag==(PARAM_a+PARAM_s+PARAM_l)){
        printf("%3ld ", buf.st_blocks/2);
        ls_l(buf,name,filecolor);
    }
    else if(flag==(PARAM_a+PARAM_i+PARAM_l)){
        printf("%7ld ", buf.st_ino);
        ls_l(buf,name,filecolor);
    }
    else if(flag==(PARAM_a+PARAM_i+PARAM_s)){
        printf("%7ld ", buf.st_ino);
        printf("%3ld ", buf.st_blocks/2);
    }
    else if(flag==(PARAM_a+PARAM_i+PARAM_s+PARAM_l)){
        printf("%7ld ", buf.st_ino);
        printf("%3ld ", buf.st_blocks/2);
        ls_l(buf,name,filecolor);
    }
}

void display_only(const char *name,int filecolor){
    char colorname[NAME_MAX+1];
    if (g_leave_len < g_maxlen){
        printf("\n");
        g_leave_len = MAXROWLEN;
    }
    int len=strlen(name);
    len=g_maxlen-len;
    sprintf(colorname,"\033[%dm%s\033[0m",filecolor,name);
    printf(" %-s",colorname);
    printf(" ");
    g_leave_len -= (g_maxlen + 2);
}

void display_dir(int flag, char *path){
    DIR *dir;
    struct dirent *entry;
    struct stat buf;
    int count=0,len=0,total=0;
    
    
    
    //打开目录并获取最长文件名   目录中文件个数 
    dir=opendir(path);
    if(dir==NULL){
        err("opendir",__LINE__);
    }
    while ((entry=readdir(dir))!=NULL)
    {
        len=strlen(entry->d_name);
        if(g_maxlen<len)   g_maxlen=len;
        count++;
    }
    closedir(dir);

    //filename声明及初始化
    char **filename=(char**)malloc(sizeof(char*)*(count));   //
    for(int i=0;i<count;i++){
        filename[i]=(char*)malloc(sizeof(char)*len);
    }
    long *filetime=(long)malloc(sizeof(long)*count);
    
    dir=opendir(path);
    if(dir==NULL){
        err("opendir",__LINE__);
    }
    for(int i=0;i<count;i++){
        strncpy(filename[count], path, strlen(path));
        filename[count][strlen(path)] = '/'; // 添加目录分隔符（在Unix/Linux上是'/'，在Windows上可能是'\'）
        strcat(filename[count], entry->d_name);
        filename[count][strlen(path)+strlen(entry->d_name)] = '\0'; // 确保字符串以空终止符结尾
        
    }
    closedir(dir);


    //涉及t按时间排序  无则按首字母排序
    if(flag&PARAM_t){
        flag-=PARAM_t;
        for(int i=0;i<count;i++){
            if(latat(filename[i],&buf)==-1){
                err("lstat",__LINE__);
            }
            filetime[i]=buf.st_mtime;
        }

        for(int i=0;i<count;i++){
            for(int j=i;j<count;j++){
                if(filetime[j]>filetime[i]){
                    char tmp[1024];
                        strcpy(tmp,filename[i]);
                        strcpy(filename[i],filename[j]);
                        strcpy(filename[j],tmp);
                }
            }
        }
    }
    else{
        for(int i=0;i<count;i++){
            for(int j=i;j<count;j++){
                if(strcmp(filename[i],filename[j])>0){
                    char tmp[1024];
                    strcpy(tmp,filename[i]);
                    strcpy(filename[i],filename[j]);
                    strcpy(filename[j],tmp);
                }
            }
        }
    }

    
    //总计  含a及不含   
    for(int i=0;i<count;i++){
        if(latat(filename[i],&buf)==-1){
                err("lstat",__LINE__);
            }
        if(flag&PARAM_a){
            total+=buf.st_blocks/2;
        }
        else{
            if(filename[i][0]!='.'){
                total+=buf.st_blocks/2; 
            }
        }
    }

    if(flag&PARAM_l ||flag&PARAM_s){
        printf("总计  %d\n",total);
    }

    if(flag&PARAM_R){
        if(flag&PARAM_r){
            flag=flag-PARAM_R-PARAM_r;//000000
            ls_R(path,flag);
        }else{
             flag=flag-PARAM_R;
             ls_R(path,flag);
        }
    }else{
        if(flag&PARAM_r){
            flag=flag-PARAM_r;
            for(int i=count-1;i>=0;i++){
                display_file(flag,filename[i]);
            }
        }else{
            for(int i=0;i<count;i++){
                display_file(flag,filename[i]);
            }
        }
    }

    //释放
    for(int i=0;i<count+1;i++){
        free(filename[i]);
    }
    free(filename);
    free(filetime);
    
}
