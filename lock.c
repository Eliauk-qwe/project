#include "lock.h"
#include <stdio.h>

void amountInit(lock_t* Account) {
  Account->amount = 0; // 初始化金额为0
  pthread_mutex_init(&Account->mutex, NULL); // 初始化互斥锁
}

void Income(lock_t* Account, int amount) {
  pthread_mutex_lock(&Account->mutex); // 加锁
  Account->amount += amount;          // 原子性增加金额
  pthread_mutex_unlock(&Account->mutex); // 解锁
}

void Expend(lock_t* Account, int amount) {
  pthread_mutex_lock(&Account->mutex); // 加锁
  Account->amount -= amount;          // 原子性减少金额
  pthread_mutex_unlock(&Account->mutex); // 解锁
}