/*
 * Semaphore.h
 *
 *  Created on: Mar 16, 2016
 *      Author: Saman Barghi
 */

#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#if defined(__linux__)

#include <semaphore.h>
#include <time.h>
#include <errno.h>

class semaphore
{
public:
    semaphore()
    {
        sem_init(&sem_, 0, 0);
    }

    ~semaphore()
    {
        while(sem_destroy(&sem_) != 0);
    }

    void wait()
    {
        while(sem_wait(&sem_) != 0);
    }
    bool trywait(){
        return (sem_trywait(&sem_) == 0);
    }

    bool timedwait(struct timespec &ts){
        while(sem_timedwait(&sem_, &ts) == -1){
            if(errno == ETIMEDOUT)
                return true;
        }
        return false;
    }

    void post()
    {
        while(sem_post(&sem_) != 0);
    }


private:
    sem_t               sem_;

    semaphore(semaphore const&);
    semaphore& operator = (semaphore const&);
};
#else
#error undefined platform: only __linux__ supported at this time
#endif
#endif /* SEMAPHORE_H_ */
