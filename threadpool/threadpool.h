#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

template <class T>
class threadpool{
public:
    threadpool(int actor_model,connection_pool * connPool,int thread_number = 8,int max_request = 10000);
    ~threadpool();
    bool append(T *request,int state);
    bool append_p(T *request);
private:
    static void *_worker(void *arg);
    void _run();
private:
    int _thread_num;            //线程池中的线程数
    int _max_requests;          //请求队列中允许的最大请求数
    pthread_t *_threads;        //描述线程池的数组，其大小为m_thread_number
    std::list<T*>_work_queue;   //请求队列
    locker _queue_locker;       //保护请求队列的互斥锁
    sem _queue_stat;            //是否有任务需要处理
    connection_pool *_conn_pool;//数据库
    int _actor_model;           //模型切换
};

template <class T>
threadpool<T>::threadpool(int actor_model,connection_pool * connPool,int thread_number = 8,int max_request = 10000){
    if (_thread_num <= 0 || _max_requests <= 0){
        throw std::exception();
    }
    _threads = new pthread_t[_thread_num];
    if (!_threads){
        throw std::exception();
    }
    for (int i = 0;i<_thread_num;++i){
        if (pthread_create(_threads+i,NULL,_worker,this)!=0){
            delete[] _threads;
            throw std::exception();
        }
        if(pthread_detach(_threads[i])){
            delete[] _threads;
            throw std::exception();
        }
        
    }
}
template <class T>
threadpool<T>::~threadpool(){
    delete[] _threads;
}
template <class T>
bool threadpool<T>::append(T *request,int state){
    _queue_locker.lock();
    if (_work_queue.size()>=_max_requests){
        _queue_locker.unlock();
        return false;
    }
    request->state = state;
    _work_queue.push_back(request);
    _queue_locker.unlock();
    _queue_stat.post();
    return true;
}

template <class T>
bool threadpool<T>::append_p(T*request){
    _queue_locker.lock();
    if (_work_queue.size()>=_max_requests){
        _queue_locker.unlock();
        return false;
    }
    _work_queue.push_back(request);
    _queue_locker.unlock();
    _queue_stat.post();
    return true;
}

template <class T>
void *threadpool<T>::_worker(void *arg){
    threadpool * pool = (threadpool *)arg;
    pool->_run();
    return pool;
}
template<class T>
void threadpool<T>::_run(){
    while(true){
        _queue_stat.wait();
        _queue_locker.lock();
        if(_work_queue.empty()){
            _queue_locker.unlock();
            continue;
        }
        T*request = _work_queue.front();
        _work_queue.pop_front();
        _queue_locker.unlock();
        if(!request){
            continue;
        }
        if(_actor_model==1){
            request->improv = 1;
            if(request->state==0){
                if (request->read_once()){
                    connectionRAII mysqlcon(&request->mysql,_conn_pool);
                    request->process();
                }else{
                    request->timer_flag = 1;
                }
            }else{
                if(!request->write()){
                    request->timer_flag = 1;
                }
            }
        }else{
            connectionRAII mysqlcon(&request->mysql,m_connPool);
            request->process();
        }
    }
}
#endif