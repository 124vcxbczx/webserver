//
// Created by liqm on 23-2-11.
//

#ifndef WEBSERVER_THREADPOOL_H
#define WEBSERVER_THREADPOOL_H

#include <pthread.h>
#include <iostream>
#include <list>
#include "locker.h"
//处理不同的任务 设为模板
template<typename T>
class threadpool{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    //添加任务的方法
    bool append(T* request);

private:
    //线程的数量
    int m_thread_number;
    //线程池
    pthread_t *m_threads;
    //请求队列中最多允许的，等待处理的请求数量
    int m_max_requests;
    //请求队列
    std::list<T*> m_workqueue;
    //互斥锁
    locker m_queuelocker;

    //信号量判断是否有任务需要处理
    sem m_queuestat;

    //是否结束线程
    bool m_stop;

    static void* worker(void* arg);
    void run();
};

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests):
    m_thread_number(thread_number),m_max_requests(max_requests),
    m_stop(false),m_threads(NULL){
    if((thread_number <= 0) || (max_requests <= 0) ) {
        throw std::exception();
    }

    m_threads = new pthread_t[m_thread_number];
    if(!m_threads) {
        throw std::exception();
    }

    // 创建thread_number 个线程，并将他们设置为脱离线程。
    for(int i = 0; i< thread_number; i++){
        std::cout<<"creat the "<<i<<" thread"<<std::endl;
        //必须是一个静态函数
        if(pthread_create(m_threads + i, NULL, worker,this )){
            throw std::exception();
        }

        if(pthread_detach(m_threads[i])){
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
threadpool<T>::~threadpool() {
    delete[] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T *request) {
    //保证线程同步 先上锁
    m_queuelocker.lock();
    //判断是否还能增加
    if(m_workqueue.size() > m_max_requests){
        //解锁
        m_queuelocker.unlock();
        return false;
    }
    //增加一个任务
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    //信号量加一个
    m_queuestat.post();
    return true;
}

template<typename T>
void* threadpool<T>::worker(void *arg) {
    threadpool* pool = ( threadpool* )arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run() {
    while(!m_stop){
        //信号量 有无任务需要完成，有的话就-1往下执行，没有就阻塞
        m_queuestat.wait();
        //上锁
        m_queuelocker.lock();
        //判断请求队列是否为空
        if(m_workqueue.empty()){
            m_queuelocker.unlock();
            continue;
        }
        //拿出请求队列的一个请求
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        //请求队列解锁
        m_queuelocker.unlock();

        //判断请求是不是空的，不是空的需要处理
        if(!request){
            continue;
        }
        request->process();
    }
}

#endif //WEBSERVER_THREADPOOL_H
