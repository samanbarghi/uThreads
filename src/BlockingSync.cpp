/*
 * BlockingSync.cpp
 *
 *  Created on: Nov 10, 2014
 *      Author: Saman Barghi
 */

#include "BlockingSync.h"
#include <iostream>

bool BlockingQueue::suspend(std::mutex& lock) {

    kThread::currentKT->currentUT->suspend(this, lock);
    //TODO: we do not have any cancellation yet, so this line will not be reached at all
    return true;
}

bool BlockingQueue::suspend(Mutex& mutex) {
    kThread::currentKT->currentUT->suspend(this, mutex);
    return true;
}

bool BlockingQueue::signal(std::mutex& lock, uThread*& owner) {
    //TODO: handle cancellation
    if (queue.front() != queue.fence()) {//Fetch one thread and put it back to ready queue
        owner = queue.front();					//FIFO?
        queue.pop_front();
        lock.unlock();
//		printAll();
        owner->resume();
        return true;
    }
    return false;
}

bool BlockingQueue::signal(Mutex& mutex) {
    //TODO: handle cancellation
    uThread* owner = nullptr;
    if (!queue.empty()) {	//Fetch one thread and put it back to ready queue
        owner = queue.front();					//FIFO?
        queue.pop_front();
        mutex.release();
        owner->resume();
        return true;
    }
    return false;
}

void BlockingQueue::signalAll(Mutex& mutex) {

    uThread* ut = queue.front();
    for (;;) {
        if (slowpath(ut == queue.fence())) break;
        queue.remove(ut);
        ut->resume();						//Send ut back to the ready queue
        ut = queue.front();
    }
    mutex.release();
}
