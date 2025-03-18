#ifndef MYTBF_H__
#define MYTBF_H__

#define MYTBF_MAX  1024

typedef struct mytbf_t mytbf_t;  // 正确定义不透明指针类型
mytbf_t *mytbf_init(int cps, int burst);  // 统一函数名前缀为 mytbf
int mytbf_fetchtoken(mytbf_t *, int);
int mytbf_returntoken(mytbf_t *, int);
int mytbf_destroy(mytbf_t *);  // 修正拼写错误

#endif