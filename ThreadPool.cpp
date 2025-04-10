#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <queue>
#include <unistd.h>

class jcn{
    public:
    int n;
    unsigned long long res;

    jcn(int n): n(n),res(1)  {}
};


class Task{
public:
    void (*func) (void*);
    void* arg;
    void (*callback) (void*);

    Task(void (*f) (void*),void *a,void (*cb) (void*))  : func(f),arg(a),callback(cb){}


};

class ThreadPool{
private:
    int pthread_count;
    pthread_t* threads;
    std::queue<Task> take_queue;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    pthread_cond_t queue_done_cond;
    bool stop;
    int not_done_pthread;

    static void* work (void* arg){
        ThreadPool* pool =static_cast<ThreadPool*> (arg);
        while (1)
        {
            pthread_mutex_lock(&pool->queue_mutex);

            while(pool->take_queue.empty() && !pool->stop){
                pthread_cond_wait(&pool->queue_cond,&pool->queue_mutex);
            }            


            if(!pool->take_queue.empty()){
                Task task =pool->take_queue.front();
                pool->take_queue.pop();
                pthread_mutex_unlock(&pool->queue_mutex);

                task.func(task.arg);
                if(task.callback){
                    task.callback(task.arg);
                }

                pthread_mutex_lock(&pool->queue_mutex);
                pool->not_done_pthread--;
                if(pool->not_done_pthread==0){
                    pthread_cond_signal(&pool->queue_done_cond);
                }
                pthread_mutex_unlock(&pool->queue_mutex);
            }
            else{
                pthread_mutex_unlock(&pool->queue_mutex);
            }


            if(pool->stop && pool->take_queue.empty()){
                pool->not_done_pthread--;
                if(pool->not_done_pthread==0){
                    pthread_cond_signal(&pool->queue_done_cond);
                }
                pthread_mutex_unlock(&pool->queue_mutex);
                break;
            }
        }
    }


public:

    ThreadPool(int num) : pthread_count(num),stop(false),not_done_pthread(0){
        pthread_mutex_init(&queue_mutex,nullptr);
        pthread_cond_init(&queue_cond,nullptr);
        pthread_cond_init(&queue_done_cond,nullptr);

        threads=new pthread_t[num];

        for(int i=0;i<num;i++){
            pthread_create(&threads[i],nullptr,work,this);
        }       
   }

   void addTask(const Task & task){
      pthread_mutex_lock(&queue_mutex);
      take_queue.push(task);
      not_done_pthread++;
      pthread_cond_signal(&queue_cond);
      pthread_mutex_unlock(&queue_mutex);
   }

   void waitpthread(){
        pthread_mutex_lock(&queue_mutex);
        if(not_done_pthread>0){
            pthread_cond_wait(&queue_done_cond,&queue_mutex);
        }
        pthread_mutex_unlock(&queue_mutex);

   }

   ~ThreadPool(){
        stop=true;
        pthread_cond_broadcast(&queue_cond);
        waitpthread();

        delete[] threads;

        pthread_mutex_destroy(&queue_mutex);
        pthread_cond_destroy(&queue_cond);
        pthread_cond_destroy(&queue_done_cond);
   }
};

void jc(void* n){
    jcn* fn =static_cast<jcn*>(n);
    for(int i=2;i<fn->n;i++){
        fn->res*=i;
    }

}

void printing(void *res){
    jcn* fres=static_cast<jcn*>(res);
    std::cout << fres->n << "的阶乘是" << fres->res << std::endl;
    delete fres;

}





int main(){
    ThreadPool pool(10);

    for(int i=0;i<20;i++){
        jcn* a =new jcn (i+1);
        pool.addTask(Task(jc,a,printing));

    }

    return 0;

}