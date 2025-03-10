#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX 128

int pass = 0; // 标记后台运行&
char lujing[1000]; // 保存当前路径

void process(char *argv[], int number);
int isdo(char *argv[], int cnt);
void mycd(char *argv[]);
void callCommandWithPipe(char *argv[], int count);
int execute(char *argv[]);
void colorprint();
void setup();
void handle_redirects(char **argv);

int main() {
    char *argv[MAX] = {NULL};
    char *cmdline = NULL;
    setup();

    while (1) {
        colorprint();
        setup();
        cmdline = readline(" ");
        if (cmdline == NULL) {
            printf("\n");
            continue;
        }
        if (strlen(cmdline) > 0)
            add_history(cmdline);

        char *mark = " ";
        int i = 1;
        argv[0] = strtok(cmdline, mark);
        while ((argv[i] = strtok(NULL, mark))) {
            i++;
        }
        process(argv, i);
        free(cmdline);
    }
    return 0;
}

void colorprint() {
    char *name = "xxx@xxx";
    printf("\033[1m\033[32m%s\033[0m", name);
    printf(":");
    getcwd(lujing, sizeof(lujing));
    printf("\033[1m\033[34m%s\033[0m", lujing);
    printf("$ ");
    fflush(stdout);
}

void setup() {
    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
}

void process(char *argv[], int cnt) {
    int flag = isdo(argv, cnt);
    if (pass == 1) {
        cnt--;
    }

    if (flag == 1) {
        mycd(argv);
    } else if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    } else if (flag == 3) {
        callCommandWithPipe(argv, cnt);
    } else {
        execute(argv);
    }
}

int execute(char *argv[]) {
    int pid;
    int child_info = -1;

    if (argv[0] == NULL)
        return 0;

    if ((pid = fork()) == -1) {
        perror("fork");
    } else if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        handle_redirects(argv);
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    } else {
        if (pass == 1) {
            pass = 0;
            printf("%d\n", pid);
            return child_info;
        }
        wait(&child_info);
    }
    return child_info;
}

int isdo(char *argv[], int cnt) {
    int flag = 10;
    if (argv[0] == NULL)
        return 0;

    if (strcmp(argv[0], "cd") == 0)
        return 1;

    for (int i = 0; i < cnt; i++) {
        if (strcmp(argv[i], "|") == 0)
            flag = 3;
        if (strcmp(argv[i], "&") == 0) {
            pass = 1;
            argv[i] = NULL;
        }
    }
    return flag;
}

void mycd(char *argv[]) {
    static char prev_dir[1024];
    char current_dir[1024];

    if (argv[1] == NULL) {
        chdir(getenv("HOME"));
    } else if (strcmp(argv[1], "-") == 0) {
        getcwd(current_dir, sizeof(current_dir));
        chdir(prev_dir);
        strcpy(prev_dir, current_dir);
    } else {
        getcwd(prev_dir, sizeof(prev_dir));
        chdir(argv[1]);
    }
}

void handle_redirects(char **argv) {
    int i;
    int in_fd, out_fd;

    for (i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], "<") == 0) {
            if (argv[i + 1] == NULL) {
                fprintf(stderr, "syntax error near '<'\n");
                exit(1);
            }
            in_fd = open(argv[i + 1], O_RDONLY);
            if (in_fd == -1) {
                perror("open");
                exit(1);
            }
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
            argv[i] = NULL;
            i++;
            while (argv[i + 1] != NULL) {
                argv[i] = argv[i + 1];
                i++;
            }
            argv[i] = NULL;
            break;
        } else if (strcmp(argv[i], ">") == 0) {
            if (argv[i + 1] == NULL) {
                fprintf(stderr, "syntax error near '>'\n");
                exit(1);
            }
            out_fd = open(argv[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (out_fd == -1) {
                perror("open");
                exit(1);
            }
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
            argv[i] = NULL;
            i++;
            while (argv[i + 1] != NULL) {
                argv[i] = argv[i + 1];
                i++;
            }
            argv[i] = NULL;
            break;
        } else if (strcmp(argv[i], ">>") == 0) {
            if (argv[i + 1] == NULL) {
                fprintf(stderr, "syntax error near '>>'\n");
                exit(1);
            }
            out_fd = open(argv[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0666);
            if (out_fd == -1) {
                perror("open");
                exit(1);
            }
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
            argv[i] = NULL;
            i++;
            while (argv[i + 1] != NULL) {
                argv[i] = argv[i + 1];
                i++;
            }
            argv[i] = NULL;
            break;
        }
    }
}

void callCommandWithPipe(char *argv[], int count) {
    int pipe_pos[10] = {0};
    int pipe_count = 0;
    pid_t pid;
    int i;

    // 查找所有管道位置
    for (i = 0; i < count; i++) {
        if (strcmp(argv[i], "|") == 0) {
            pipe_pos[pipe_count++] = i;
        }
    }

    int cmd_count = pipe_count + 1;
    char *cmds[cmd_count][MAX];
    int cmd_index = 0;
    int start = 0;

    // 分割命令
    for (i = 0; i < cmd_count; i++) {
        int end = (i < pipe_count) ? pipe_pos[i] : count;
        int argc = 0;
        for (int j = start; j < end; j++) {
            cmds[i][argc++] = argv[j];
        }
        cmds[i][argc] = NULL;
        start = end + 1;
    }

    int fd[pipe_count][2];
    for (i = 0; i < pipe_count; i++) {
        if (pipe(fd[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    for (i = 0; i < cmd_count; i++) {
        pid = fork();
        if (pid == 0) {
            // 子进程处理管道
            if (i < cmd_count - 1) {
                dup2(fd[i][1], STDOUT_FILENO);
            }
            if (i > 0) {
                dup2(fd[i-1][0], STDIN_FILENO);
            }
            // 关闭所有管道描述符
            for (int j = 0; j < pipe_count; j++) {
                close(fd[j][0]);
                close(fd[j][1]);
            }
            // 处理重定向
            handle_redirects(cmds[i]);
            execvp(cmds[i][0], cmds[i]);
            perror("execvp");
            exit(1);
        } else if (pid < 0) {
            perror("fork");
            exit(1);
        }
    }

    // 父进程关闭所有管道描述符
    for (i = 0; i < pipe_count; i++) {
        close(fd[i][0]);
        close(fd[i][1]);
    }

    // 等待所有子进程
    if (pass == 1) {
        pass = 0;
        printf("%d\n", pid);
        return;
    }
    for (i = 0; i < cmd_count; i++) {
        wait(NULL);
    }
}