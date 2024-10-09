#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

class sem
{
public:
    sem(int num){
        if(sem_init(&_sem,0,num)!=0){
            throw std::exception();
        }
    }
    sem():sem(0){};
    ~sem(){
        sem_destroy(&_sem);
    }
    bool wait(){
        return sem_wait(&_sem)==0;
    }
    bool post(){

    }
private:
    sem_t _sem;
}