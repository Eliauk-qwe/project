#include <pthread.h>
#include <queue>
#include <functional>

using namespace std;

struct Task{
    using funcv = std :: function<void()>;

    Task() =default;
    Task(funcv f) : function(std::move(f)){}

    funcv function;

    void execute(){
        if(function)  function();
    }


};

class ThreadPool{
    public:
    //初始化（构造）线程池
    ThreadPool(size_t number){
        stop=false;

        pthread_mutex_init(&mutex,nullptr);
        pthread_cond_init(&cond,nullptr);

        workers.resize(number);

        for(size_t i=0;i<number;i++){
            pthread_create(&workers[i],nullptr,work,this);
        }

    }


    //添加任务
    void addTask(const Task &task){
        pthread_mutex_lock(&mutex);
        taskqueue.push(task);
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&cond);
    }

    ~ThreadPool(){
        pthread_mutex_lock(&mutex);
        stop=true;
        pthread_mutex_unlock(&mutex);

        pthread_cond_broadcast(&cond);

        for(pthread_t &work :workers){
            pthread_join(work,nullptr);
        }

        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);

    }


    private:

    //成员
    vector<pthread_t> workers;//存储所有工作线程ID
    queue<Task> taskqueue;     //任务队列
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool stop;


    //工作线程

    static void* work (void* arg){
        ThreadPool* pool =static_cast<ThreadPool*>(arg);
        while(1){
             pthread_mutex_lock(&pool->mutex);

            while(!pool->stop && pool->taskqueue.empty()){
                pthread_cond_wait(&pool->cond,&pool->mutex);
            }

           if(pool->stop && pool->taskqueue.empty()){
              pthread_mutex_unlock(&pool->mutex);
           }

           Task task =pool->taskqueue.front();
           pool->taskqueue.pop();
           pthread_mutex_unlock(&pool->mutex);

           task.execute();
        }

        return nullptr;

    }




};


/*
// 1. 使用lambda表达式创建任务
Task task1([]() {
    std::cout << "Task 1 executed\n";
});

// 2. 使用函数指针创建任务
void myFunction() {
    std::cout << "My function executed\n";
}
Task task2(myFunction);

// 3. 使用函数对象创建任务
struct MyFunctor {
    void operator()() const {
        std::cout << "Functor executed\n";
    }
};
Task task3(MyFunctor{});

// 执行任务
task1.execute(); // 输出: Task 1 executed
task2.execute(); // 输出: My function executed
task3.execute(); // 输出: Functor executed
*/


