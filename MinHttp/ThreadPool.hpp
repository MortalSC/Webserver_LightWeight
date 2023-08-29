#pragma once

#include <iostream>
#include <queue>
#include <pthread.h>
#include "Task.hpp"
#include "Log.hpp"

#define NUM 6

class ThreadPool
{
private:
    int _num;                    // 线程池中线程个数
    bool _stop;                  // 线程池运行状态
    std::queue<Task> task_queue; // 任务队列
    pthread_mutex_t _lock;       // 互斥锁
    pthread_cond_t _cond;        // 条件变量

    ThreadPool(int num = NUM) : _num(num), _stop(false)
    {
        pthread_mutex_init(&_lock, nullptr);
        pthread_cond_init(&_cond, nullptr);
    }

    ThreadPool(const ThreadPool &tp) {}
    static ThreadPool *single_instance; // 单例指针
public:
    static ThreadPool *GetInstance()
    {
        pthread_mutex_t mymutex = PTHREAD_MUTEX_INITIALIZER;
        if (single_instance == nullptr)
        {

            pthread_mutex_lock(&mymutex);
            if (single_instance == nullptr)
            {
                single_instance = new ThreadPool();
                single_instance->InitThreadPool();
            }
            pthread_mutex_unlock(&mymutex);
        }
        return single_instance;
    }

    // 线程的例程
    static void *ThreadRoutine(void *args)
    {
        // 静态方法不能访问类内成员
        ThreadPool *tp = (ThreadPool *)args;

        while (true)
        {
            Task task;
            // 从任务队列中获取任务
            tp->Lock();
            // if(tp->TaskQueueIsEmpty()){
            while (tp->TaskQueueIsEmpty())
            { // 避免伪唤醒的情况，导致一个拿到任务，其他没有导致的报错
                // 队列为空进行等待！
                tp->ThreadWait(); // 当线程被唤醒，一定是占有互斥锁的！
            }
            tp->PopTask(task);
            tp->UnLock();

            // 处理任务
            task.ProcessOn();
        }
    }

    // 启动线程池（线程初始化）
    bool InitThreadPool()
    {
        for (int i = 0; i < _num; i++)
        {
            pthread_t tid;
            if (pthread_create(&tid, nullptr, ThreadRoutine, this) != 0)
            {
                // 线程创建失败！
                LogMessage(FATAL, "Create ThreadPool Fail");
                return false;
            }
            else
            {
            }
        }
        LogMessage(INFO, "Create ThreadPool Success");
        return true;
    }

    // 任务队列是否为空！
    bool TaskQueueIsEmpty()
    {
        return task_queue.empty();
    }

    // 插入任务
    void PushTask(const Task &task)
    {
        Lock();
        task_queue.push(task);
        UnLock();
        ThreadWakeup();
    }

    // 剔除任务
    void PopTask(Task &task)
    {
        task = task_queue.front();
        task_queue.pop();
    }

    // 线程加锁
    void Lock()
    {
        pthread_mutex_lock(&_lock);
    }

    // 线程解锁
    void UnLock()
    {
        pthread_mutex_unlock(&_lock);
    }

    // 线程等待
    void ThreadWait()
    {
        // 等待中：先放下锁，需要时再唤醒
        pthread_cond_wait(&_cond, &_lock);
    }

    // 线程唤醒
    void ThreadWakeup()
    {
        // 唤醒
        pthread_cond_signal(&_cond);
    }

    bool IsStop()
    {
        return _stop;
    }

    ~ThreadPool()
    {
        pthread_mutex_destroy(&_lock);
        pthread_cond_destroy(&_cond);
    }
};

ThreadPool *ThreadPool::single_instance = nullptr;