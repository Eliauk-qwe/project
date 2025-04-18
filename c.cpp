#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <queue>
#include <unistd.h>

class jcn {
public:
    int n;
    unsigned long long res;

    jcn(int n) : n(n), res(1ull) {}  // 使用ull明确无符号长整型
};

class Task {
public:
    void (*func)(void*);
    void* arg;
    void (*callback)(void*);

    Task(void (*f)(void*), void* a, void (*cb)(void*)) 
        : func(f), arg(a), callback(cb) {}
};

class ThreadPool {
private:
    int pthread_count;
    pthread_t* threads;
    std::queue<Task> task_queue;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    pthread_cond_t queue_done_cond;
    bool stop;
    int not_done_tasks;
    static pthread_mutex_t print_mutex;  // 添加输出专用锁

    static void* work(void* arg) {
        ThreadPool* pool = static_cast<ThreadPool*>(arg);
        while (true) {
            pthread_mutex_lock(&pool->queue_mutex);

            while (pool->task_queue.empty() && !pool->stop) {
                pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
            }

            if (!pool->task_queue.empty()) {
                Task task = pool->task_queue.front();
                pool->task_queue.pop();
                pthread_mutex_unlock(&pool->queue_mutex);

                task.func(task.arg);     // 执行计算任务
                if (task.callback) {
                    task.callback(task.arg);  // 执行回调
                }

                pthread_mutex_lock(&pool->queue_mutex);
                pool->not_done_tasks--;
                if (pool->not_done_tasks == 0) {
                    pthread_cond_signal(&pool->queue_done_cond);
                }
                pthread_mutex_unlock(&pool->queue_mutex);
            } else {
                pthread_mutex_unlock(&pool->queue_mutex);
                if (pool->stop) break;
            }
        }
        return nullptr;
    }

public:
    ThreadPool(int num) : pthread_count(num), stop(false), not_done_tasks(0) {
        pthread_mutex_init(&queue_mutex, nullptr);
        pthread_cond_init(&queue_cond, nullptr);
        pthread_cond_init(&queue_done_cond, nullptr);

        threads = new pthread_t[num];
        for (int i = 0; i < num; ++i) {
            pthread_create(&threads[i], nullptr, work, this);
        }
    }

    void addTask(const Task& task) {
        pthread_mutex_lock(&queue_mutex);
        task_queue.push(task);
        not_done_tasks++;
        pthread_cond_signal(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);
    }

    void waitAll() {
        pthread_mutex_lock(&queue_mutex);
        while (not_done_tasks > 0) {
            pthread_cond_wait(&queue_done_cond, &queue_mutex);
        }
        pthread_mutex_unlock(&queue_mutex);
    }

    ~ThreadPool() {
        pthread_mutex_lock(&queue_mutex);
        stop = true;
        pthread_cond_broadcast(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);

        for (int i = 0; i < pthread_count; ++i) {
            pthread_join(threads[i], nullptr);
        }

        delete[] threads;

        pthread_mutex_destroy(&queue_mutex);
        pthread_cond_destroy(&queue_cond);
        pthread_cond_destroy(&queue_done_cond);
    }

    // 提供一个公共接口来访问输出锁
    static void lockPrintMutex() {
        pthread_mutex_lock(&print_mutex);
    }

    static void unlockPrintMutex() {
        pthread_mutex_unlock(&print_mutex);
    }
};

pthread_mutex_t ThreadPool::print_mutex = PTHREAD_MUTEX_INITIALIZER;

void jc(void* arg) {
    jcn* fn = static_cast<jcn*>(arg);
    for (int i = 2; i <= fn->n; ++i) {
        fn->res *= i;
    }
}

void printing(void* arg) {
    jcn* fres = static_cast<jcn*>(arg);
    ThreadPool::lockPrintMutex();  // 使用公共接口加锁
    std::cout << fres->n << "的阶乘是" << fres->res << std::endl;
    ThreadPool::unlockPrintMutex();
    delete fres;
}

int main() {
    ThreadPool pool(10);

    for (int i = 0; i < 20; ++i) {
        jcn* a = new jcn(i+1);  // 计算0!到19!
        pool.addTask(Task(jc, a, printing));
    }

    pool.waitAll();
    return 0;
}