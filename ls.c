#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

void ls_a(const char *dir_path){
    DIR *dir;
    struct dirent *entry;
    
    
    dir=opendir(dir_path);
    if(dir==NULL){
        perror("opendir failed");
        return ;
    }

    while((entry=readdir(dir))!=NULL){
        printf("%s\n",entry->d_name);
    }

    if(closedir(dir)==-1){
        perror("closedir failed");
        return ;
    }

     


}


void   ls_l(const char*path){
    typedef struct 
    {
        mode_t    mode;
        nlink_t   links;
        uid_t     uidname;
        gid_t     gidname;
        off_t     sizes;
        time_t    time;
        char      filename[100];
    }file_xx;

    DIR *dir;
    struct dirent *entry;
    struct stat  file_stat;
    file_xx *file=NULL;
    char fullpath[1024];
    int file_alloc=10;
    int count=0,sum=0;


    dir=opendir(path);
    if(dir==NULL){
        perror("opendir fail");
        return;
    }

    file=malloc(file_alloc*sizeof(file_xx));
    if(file==NULL){
        perror("malloc faile");
        closedir(dir);
        return;
    }

    while((entry=readdir(dir))!=NULL){
        if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){
            continue;
        }

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        stat(fullpath,&file_stat);
        /*if (stat(fullpath, &file_stat) != 0) {
            perror("stat fail");
            continue; // 或者采取其他错误处理措施
        }*/

       char st_mode[11];
       mode_t mode =file_stat.st_mode;
       snprintf(st_mode,sizeof(st_mode),"%c%c%c%c%c%c%c%c%c%c",
        (S_ISDIR(mode))?'d':'-',//很多种
        (mode & S_IRUSR)?'r':'-',
        (mode & S_IWUSR)?'w':'-',
        (mode & S_IXUSR)?'x':'-',
        (mode & S_IRGRP)?'r':'-',
        (mode & S_IWGRP)?'w':'-',
        (mode & S_IXGRP)?'x':'-',
        (mode & S_IROTH)?'r':'-',
        (mode & S_IWOTH)?'w':'-',
        (mode & S_IXOTH)?'x':'-'
    );

        struct passwd *uid=getpwuid(file_stat.st_uid);
        struct group *gid=getgrgid(file_stat.st_gid);


        char time_str[30];
        struct tm *tm_info=localtime(file_stat.st_mtime);
        strftime(time_str,sizeof(time_str),"%m月 %d %H:%M",tm_info);

        
        
        if(count>=file_alloc){
            file_alloc*=2;
            file_xx *new_file=realloc(file,file_alloc*sizeof(file_xx));
            if(new_file==NULL){
                perror("realloc fail");
                free(file);
                closedir(dir);
                return;
            }
            file=new_file;

       }

       strcpy(file[count].mode,st_mode);
       file[count].links=file_stat.st_nlink;
       strcpy(file[count].uidname,uid->pw_name);
       strcpy(file[count].gidname,gid->gr_name);
       file[count].sizes=file_stat.st_size;
       strcpy(file[count].time,time_str);
       strcpy(file[count].filename,entry->d_name);

       sum=sum+file_stat.st_blocks;
       
    }

    printf("总计 %d\n",sum);

    for(int i=0;i<count;i++){
        printf("%s  %u  %s  %s  %ld  %s %s \n",file[i].mode,file[i].links,file[i].uidname,file[i].gidname,file[i].sizes,file[i].time,file[i].filename);
    }

    free(file);
    closedir(dir);
}


void ls_R(const char *path){
    DIR *dir;
    struct dirent *entry;
    char now_path[1024];
    struct stat name;
    char **is_dir;
    int i=0,count=0;



    dir=opendir(path) ;
    if(dir==NULL){
        perror("openfir fail");
        return ;
    }

    printf("%s:\n",path);

     
    while((entry=readdir(dir))!=NULL){
        if(strcmp(entry->d_name,".")==0||strcmp(entry->d_name,"..")==0){
            continue;
        }
        
        //printf("%s\n",entry->d_name);
        snprintf(now_path,sizeof(now_path),"%s/%s",path,entry->d_name);

        if(lstat(now_path, &name) == -1) {
            perror("lstat fail");
            continue;
        }
        
        

        if(S_ISDIR(name.st_mode)){
            is_dir=realloc(is_dir,(count+1)*sizeof(char*));//判断
            /*if (is_dir == NULL) {
                perror("realloc fail");
                free(is_dir);
                closedir(dir);
                return;
            }*/
            is_dir[count]=strdup(now_path);
            /*if (is_dir[count] == NULL) {
                perror("strdup fail");
                free(is_dir);
                // 注意：这里应该释放之前分配的内存，但为了简化示例，我们省略了这一步
                // 在实际代码中，您应该添加适当的错误处理和内存释放逻辑
                closedir(dir);
                return;
            }*/
            count++;
        }


    }


    for(int j=0;j<i;j++){
        ls_R(is_dir[j]);
        free(is_dir[j]);
    }
    free(is_dir);

    closedir(dir);

}



/*#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

// 假设这个函数已经实现，用于递归地列出目录内容
void list_directory(const char *path);

void ls_R(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char now_path[1024]; // 使用静态数组作为临时缓冲区，注意这可能会限制路径的长度
    struct stat name;
    char **is_dir;
    int i = 0, count = 0; // count 用于记录目录的数量

    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir fail");
        return;
    }

    printf("%s:\n", path);

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        printf("%s\n", entry->d_name);

        // 构建完整的路径
        snprintf(now_path, sizeof(now_path), "%s/%s", path, entry->d_name);

        if (lstat(now_path, &name) == -1) {
            perror("lstat fail");
            continue;
        }

        if (S_ISDIR(name.st_mode)) {
            // 动态分配内存来存储指向目录路径的指针
            is_dir = realloc(is_dir, (count + 1) * sizeof(char *));
            if (is_dir == NULL) {
                perror("realloc fail");
                closedir(dir);
                return;
            }
            is_dir[count] = strdup(now_path); // 使用 strdup 来复制字符串，并分配内存
            if (is_dir[count] == NULL) {
                perror("strdup fail");
                // 注意：这里应该释放之前分配的内存，但为了简化示例，我们省略了这一步
                // 在实际代码中，您应该添加适当的错误处理和内存释放逻辑
                closedir(dir);
                return;
            }
            count++;
        }
    }

    // 递归地列出每个子目录的内容
    for (int j = 0; j < count; j++) {
        list_directory(is_dir[j]);
        free(is_dir[j]); // 释放之前使用 strdup 分配的内存
    }
    free(is_dir); // 释放使用 realloc 分配的内存

    closedir(dir);
}

// 这里应该实现 list_directory 函数，但由于篇幅限制，我们省略了它
// 它应该接收一个路径作为参数，并列出该路径下的所有文件和目录*/

void  ls_t(const char *path){
    typedef struct  
    {
        char  filename[100];//修改
        time_t   modifytime;
    }File;
     

    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    int count=0;
    char fullpath[1024];
    File *files=NULL;
    int files_alloc=10;
    

    dir=opendir(path);
    if(dir==NULL){
        perror("opendir fail");
        return ;
    }

    //动态分配FILE数组
    files = malloc(files_alloc * sizeof(File));
    if (files == NULL) {
        perror("malloc fail");
        closedir(dir);
        return;
    }

    while ((entry=readdir(dir))!=NULL)
    {
        if(strcmp(entry->d_name,".")==0||strcmp(entry->d_name,"..")==0){
            continue;
        }
        
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);//

        stat(fullpath,&file_stat);//失败
        /*if (stat(fullpath, &file_stat) != 0) {
            perror("stat fail");
            continue; // 或者采取其他错误处理措施
        }*/

        // 如果数组已满，则重新分配更大的数组
        if (count >= files_alloc) {
            files_alloc *= 2;
            File *new_files = realloc(files, files_alloc * sizeof(File));
            if (new_files == NULL) {
                perror("realloc fail");
                free(files);
                closedir(dir);
                return;
            }
            files = new_files;
        }

        strcpy(files[count].filename,entry->d_name);
        files[count].modifytime=file_stat.st_mtime;
        count++;
    }
    
    for(int i=0; i<count-1;i++){
        for(int j=0;j<count-i-1;j++){
            if(files[j].modifytime<files[j+1].modifytime){
                //结构体互换
                File temp=files[j];
                files[j]=files[j+1];
                files[j+1]=temp;
            }
        }
    }

    for(int i=0;i<count;i++){
        printf("%s\n",files[i].filename);
    }

    free(files);
    closedir(dir);
    
}



void ls_r(const char *path){
    DIR *dir;
    struct dirent *entry;
    char **return_dir=NULL; 
    int count=0;
    int return_dir_alloc=1024;
    char **t;

    dir=opendir(path);
    if(dir==NULL){
        perror("oprndir fail");
        return ;
    }

    // 分配初始容量的内存
    return_dir = malloc(return_dir_alloc * sizeof(char *));
    if (return_dir == NULL) {
        perror("malloc fail");
        closedir(dir);
        return;
    }

    while((entry=readdir(dir))!=NULL){
        if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){
            continue;
        }

        // 如果达到容量，增加容量
        if (count == return_dir_alloc) {
            return_dir_alloc *= 2;
            t = realloc(return_dir, return_dir_alloc* sizeof(char *));
            if (t == NULL) {
                perror("realloc fail");
                // 释放已分配的内存和已复制的字符串，并关闭目录
                for (int i = 0; i < count; i++) {
                    free(return_dir[i]);
                }
                free(return_dir);
                closedir(dir);
                return;
            }
            return_dir = t;
        }
       
        return_dir[count]=strdup(entry->d_name);
        
        count++;

    }

    for(int i=count-1;i>=0;i--){
        printf("%10s",return_dir[i]);
        free(return_dir[i]);

    }
    free(return_dir);

    closedir(dir);
}



void ls_i(const char *path){
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char fullpath[1024];
    
    dir=opendir(path);
    if(dir==NULL){
        perror("opendir failed");
        return ;
    }

    while((entry=readdir(dir))!=NULL){
        if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){
            continue;
        }

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        stat(fullpath,&file_stat);
        /*if (stat(fullpath, &file_stat) != 0) {
            perror("stat fail");
            continue; // 或者采取其他错误处理措施
        }*/

        printf("%lu %s\n",file_stat.st_ino,entry->d_name);


    }

    if(closedir(dir)==-1){
        perror("closedir failed");
        return ;
    }

     


}


void ls_s(const char *path){
    typedef struct 
    {
        blkcnt_t  block;
        char      filename[100];
    }blocks;


    DIR *dir;
    struct dirent *entry;
    struct stat  file_stat;
    blocks *f=NULL;
    char fullpath[1024];
    int f_alloc=10;
    int count=0,sum=0;
    

    dir=opendir(path);
    if(dir==NULL){
        perror("opendir fail");
        return;
    }

    f=malloc(f_alloc*sizeof(blocks));
    if(f==NULL){
        perror("malloc faile");
        closedir(dir);
        return;
    }

    while((entry=readdir(dir))!=NULL){
        if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name,"..")==0){
            continue;
        }

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        stat(fullpath,&file_stat);
        /*if (stat(fullpath, &file_stat) != 0) {
            perror("stat fail");
            continue; // 或者采取其他错误处理措施
        }*/
        
        
        if(count>=f_alloc){
            f_alloc*=2;
            blocks *new_f=realloc(f,f_alloc*sizeof(blocks));
            if(new_f==NULL){
                perror("realloc fail");
                free(f);
                closedir(dir);
                return;
            }
            f=new_f;

       }

       strcpy(f[count].filename,entry->d_name);
       f[count].block=file_stat.st_blocks;
       sum=sum+file_stat.st_blocks;
       
    }

    printf("总计 %d\n",sum);

    for(int i=0;i<count;i++){
        printf("%ld  %s\n",f[i].block,f[i].filename);
    }

    free(f);
    closedir(dir);
    
}