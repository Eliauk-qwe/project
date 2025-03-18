#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include "mytbf.h"  // 头文件名修正为 mytbf.h
#include <unistd.h>

#define MYTBF_MAX 1024

static mytbf_t *job[MYTBF_MAX];
static int inited = 0;
static void (*alrm_handler_save)(int);  // 更清晰的信号处理函数指针定义

struct mytbf_t {  // 结构体名与头文件中的 mytbf_t 一致
    int cps;
    int burst;
    int token;
    int pos;
};

// 获取空闲位置的函数
static int get_free_pos() {
    for (int i = 0; i < MYTBF_MAX; i++) {
        if (job[i] == NULL) {
            return i;
        }
    }
    return -1;
}

// SIGALRM 信号处理函数
static void alrm_handler(int s) {
    alarm(1);
    for (int i = 0; i < MYTBF_MAX; i++) {
        if (job[i] != NULL) {
            job[i]->token += job[i]->cps;
            if (job[i]->token > job[i]->burst) {
                job[i]->token = job[i]->burst;
            }
        }
    }
}

// 模块卸载（清理资源）
static void module_unload() {  // 修正函数名拼写
    signal(SIGALRM, alrm_handler_save);
    alarm(0);
    for (int i = 0; i < MYTBF_MAX; i++) {
        free(job[i]);
        job[i] = NULL;
    }
}

// 模块加载（注册信号处理）
static void module_load() {  // 修正函数名拼写
    alrm_handler_save = signal(SIGALRM, alrm_handler);
    alarm(1);
    atexit(module_unload);  // 修正函数名
}



// 初始化令牌桶
mytbf_t *mytbf_init(int cps, int burst) {  // 函数名修正
    struct mytbf_t *me;
    int pos;

    if (!inited) {
        module_load();
        inited = 1;
    }

    pos = get_free_pos();
    if (pos < 0) return NULL;

    me = malloc(sizeof(*me));
    if (me == NULL) return NULL;

    me->token = 0;
    me->cps = cps;
    me->burst = burst;
    me->pos = pos;

    job[pos] = me;
    return me;
}

// 获取令牌
int mytbf_fetchtoken(mytbf_t *ptr, int size) {  // 参数类型修正
    struct mytbf_t *me = (struct mytbf_t *)ptr;
    if (size <= 0) return -EINVAL;

    while (me->token <= 0) {
        pause();  // 等待令牌补充
    }

    int n = (me->token < size) ? me->token : size;
    me->token -= n;
    return n;
}

// 归还令牌
int mytbf_returntoken(mytbf_t *ptr, int size) {  // 参数类型修正
    struct mytbf_t *me = (struct mytbf_t *)ptr;
    if (size <= 0) return -EINVAL;

    me->token += size;
    if (me->token > me->burst) {
        me->token = me->burst;
    }
    return size;
}

// 销毁令牌桶
int mytbf_destroy(mytbf_t *ptr) {  // 函数名修正
    struct mytbf_t *me = (struct mytbf_t *)ptr;
    job[me->pos] = NULL;
    free(me);
    return 0;
}