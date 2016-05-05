/*
 * Semaphore.h
 *
 *  Created on: Mar 16, 2016
 *      Author: Saman Barghi
 */

#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_
#include <semaphore.h>

#if defined(__linux__)

class semaphore
{
public:
    semaphore()
    {
        sem_init(&sem_, 0, 0);
    }

    ~semaphore()
    {
        sem_destroy(&sem_);
    }

    void wait()
    {
        sem_wait(&sem_);
    }

    void post()
    {
        sem_post(&sem_);
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
