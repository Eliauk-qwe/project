#include <stdio.h>
#include <linux/limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/ioctl.h>

#define PARAM_NONE 0
#define PARAM_a 1
#define PARAM_l 2
#define PARAM_R 4
#define PARAM_t 8
#define PARAM_r 16
#define PARAM_i 32
#define PARAM_s 64
#define MAXROWLEN 155

typedef struct {
    char name[NAME_MAX + 1];
    struct stat st;
} FileEntry;

typedef struct {
    int max_inode;
    int max_blocks;
    int max_nlink;
    int max_user;
    int max_group;
    int max_size;
    int max_name;
} ColumnWidths;

void process_path(const char *path, int flag, bool is_root);
void display_entries(FileEntry *entries, int count, int flag,const char *path);
int compare_entries(const void *a, const void *b);
void ls_l(const FileEntry *entry, ColumnWidths *widths, const char *fullpath);
void print_color_name(const char *name, mode_t mode);
void handle_error(const char *path);
void calculate_widths(FileEntry *entries, int count, ColumnWidths *widths, int flag);
void display_normal(FileEntry *entries, int count, int term_width, ColumnWidths *widths);

ColumnWidths global_widths;

int main(int argc, char *argv[]) {
    int flag = PARAM_NONE;
    char **paths = NULL;
    int path_count = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (size_t j = 1; j < strlen(argv[i]); j++) {
                switch (argv[i][j]) {
                    case 'a': flag |= PARAM_a; break;
                    case 'l': flag |= PARAM_l; break;
                    case 'R': flag |= PARAM_R; break;
                    case 't': flag |= PARAM_t; break;
                    case 'r': flag |= PARAM_r; break;
                    case 'i': flag |= PARAM_i; break;
                    case 's': flag |= PARAM_s; break;
                    default: 
                        fprintf(stderr, "无效选项: -%c\n", argv[i][j]);
                        exit(EXIT_FAILURE);
                }
            }
        } else {
            paths = realloc(paths, sizeof(char *) * (path_count + 1));
            paths[path_count] = strdup(argv[i]);
            path_count++;
        }
    }

    if (path_count == 0) {
        paths = malloc(sizeof(char *));
        paths[0] = strdup(".");
        path_count = 1;
    }

    for (int i = 0; i < path_count; i++) {
        process_path(paths[i], flag, true);
        free(paths[i]);
    }

    free(paths);
    return 0;
}

void process_path(const char *path, int flag, bool print_header) {
    struct stat stat_buf;
    if (lstat(path, &stat_buf) == -1) {
        handle_error(path);
        return;
    }

    if (S_ISDIR(stat_buf.st_mode)) {
        if (flag&PARAM_R) printf("%s:\n", path);
        DIR *dir = opendir(path);
        if (!dir) {
            handle_error(path);
            return;
        }

        FileEntry *entries = NULL;
        int entry_count = 0;
        struct dirent *dirent;
        char fullpath[PATH_MAX];

        while ((dirent = readdir(dir)) != NULL) {
            if (!(flag & PARAM_a) && dirent->d_name[0] == '.') continue;
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, dirent->d_name);
            entries = realloc(entries, (entry_count + 1) * sizeof(FileEntry));
            lstat(fullpath, &entries[entry_count].st);
            strncpy(entries[entry_count].name, dirent->d_name, NAME_MAX);
            entry_count++;
        }
        closedir(dir);

        if (flag & (PARAM_t | PARAM_r)) {
            qsort(entries, entry_count, sizeof(FileEntry), compare_entries);
            if (flag & PARAM_r) {
                for (int i = 0; i < entry_count / 2; i++) {
                    FileEntry tmp = entries[i];
                    entries[i] = entries[entry_count - i - 1];
                    entries[entry_count - i - 1] = tmp;
                }
            }
        }

        display_entries(entries, entry_count, flag, path);
        free(entries);

        if (flag & PARAM_R) {
            dir = opendir(path);
            while ((dirent = readdir(dir)) != NULL) {
                if (strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0) continue;
                snprintf(fullpath, sizeof(fullpath), "%s/%s", path, dirent->d_name);
                lstat(fullpath, &stat_buf);
                if (S_ISDIR(stat_buf.st_mode)) {
                    process_path(fullpath, flag, true);
                }
            }
            closedir(dir);
        }
    } else {
        FileEntry entry;
        strncpy(entry.name, path, NAME_MAX);
        entry.st = stat_buf;
        display_entries(&entry, 1, flag, path);
    }
}
int compare_entries(const void *a, const void *b) {
    const FileEntry *ea = (const FileEntry *)a;
    const FileEntry *eb = (const FileEntry *)b;
    if (ea->st.st_mtime != eb->st.st_mtime)
        return (eb->st.st_mtime > ea->st.st_mtime) ? 1 : -1;
    return strcasecmp(ea->name, eb->name);
}

void calculate_widths(FileEntry *entries, int count, ColumnWidths *widths, int flag) {
    memset(widths, 0, sizeof(ColumnWidths));
    for (int i = 0; i < count; i++) {
        // 计算inode列宽
        if (flag & PARAM_i) {
            int len = snprintf(NULL, 0, "%lu", entries[i].st.st_ino);
            if (len > widths->max_inode) widths->max_inode = len;
        }
        // 计算块大小列宽
        if (flag & PARAM_s) {
            int len = snprintf(NULL, 0, "%ld", entries[i].st.st_blocks/2);
            if (len > widths->max_blocks) widths->max_blocks = len;
        }
    }
}

void display_entries(FileEntry *entries, int count, int flag,const char *path) {
    if (count == 0) return;

    ColumnWidths widths;
    calculate_widths(entries, count, &widths, flag);

    // 显示总用量（-s参数）
    if (flag & (PARAM_l | PARAM_s)) {
        long total = 0;
        for (int i = 0; i < count; i++) total += entries[i].st.st_blocks;
        printf("总用量 %ld\n", total/2);
    }

    // 长格式输出
    if (flag & PARAM_l) {
        for (int i = 0; i < count; i++) {
            // 显示inode号
            if (flag & PARAM_i) printf("%*lu ", widths.max_inode, entries[i].st.st_ino);
            // 显示块大小
            if (flag & PARAM_s) printf("%*ld ", widths.max_blocks, entries[i].st.st_blocks/2);
            
            // 文件类型和权限
            printf("%c", S_ISDIR(entries[i].st.st_mode) ? 'd' : 
                   S_ISLNK(entries[i].st.st_mode) ? 'l' : '-');
            printf("%c%c%c", entries[i].st.st_mode & S_IRUSR ? 'r' : '-',
                             entries[i].st.st_mode & S_IWUSR ? 'w' : '-',
                             entries[i].st.st_mode & S_IXUSR ? 'x' : '-');
            printf("%c%c%c", entries[i].st.st_mode & S_IRGRP ? 'r' : '-',
                             entries[i].st.st_mode & S_IWGRP ? 'w' : '-',
                             entries[i].st.st_mode & S_IXGRP ? 'x' : '-');
            printf("%c%c%c ", entries[i].st.st_mode & S_IROTH ? 'r' : '-',
                              entries[i].st.st_mode & S_IWOTH ? 'w' : '-',
                              entries[i].st.st_mode & S_IXOTH ? 'x' : '-');

            // 其他信息
            struct passwd *pw = getpwuid(entries[i].st.st_uid);
            struct group *gr = getgrgid(entries[i].st.st_gid);
            printf("%3lu %-8s %-8s %8ld ",
                   entries[i].st.st_nlink,
                   pw ? pw->pw_name : "unknown",
                   gr ? gr->gr_name : "unknown",
                   entries[i].st.st_size);

            // 时间格式化
            char time_buf[80];
            strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", localtime(&entries[i].st.st_mtime));
            printf("%s ", time_buf);

            print_color_name(entries[i].name, entries[i].st.st_mode);
            printf("\n");
        }
    } else { // 普通模式输出
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        display_normal(entries, count, w.ws_col, &widths);
    }
}

void display_normal(FileEntry *entries, int count, int term_width, ColumnWidths *widths) {
    int max_name_len = 0;
    for (int i = 0; i < count; i++) {
        int len = strlen(entries[i].name);
        if (len > max_name_len) max_name_len = len;
    }

    // 计算额外字段宽度
    int extra_width = 0;
    if (widths->max_inode > 0) extra_width += widths->max_inode + 1;
    if (widths->max_blocks > 0) extra_width += widths->max_blocks + 1;

    int col_width = max_name_len + extra_width + 2;
    int cols = term_width / col_width;
    if (cols == 0) cols = 1;
    int rows = (count + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = c * rows + r;
            if (idx >= count) continue;
            
            // 打印inode和块大小
            if (widths->max_inode > 0) 
                printf("%*lu ", widths->max_inode, entries[idx].st.st_ino);
            if (widths->max_blocks > 0) 
                printf("%*ld ", widths->max_blocks, entries[idx].st.st_blocks/2);
            
            // 打印带颜色的文件名
            print_color_name(entries[idx].name, entries[idx].st.st_mode);
            
            // 填充空格对齐
            int spaces = col_width - strlen(entries[idx].name) - extra_width;
            printf("%*s", spaces, "");
        }
        printf("\n");
    }
}

void print_color_name(const char *name, mode_t mode) {
    if (S_ISDIR(mode)) printf("\033[1;34m");
    else if (S_ISLNK(mode)) printf("\033[1;36m");
    else if (mode & S_IXUSR) printf("\033[1;32m");
    else printf("\033[0m");
    printf("%s\033[0m", name);
}

void handle_error(const char *path) {
    fprintf(stderr, "ls: 无法访问 '%s': ", path);
    perror("");
    if (errno == EACCES) fprintf(stderr, "权限被拒绝\n");
}