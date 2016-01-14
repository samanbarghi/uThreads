/*
 * ReadyQueue.h
 *
 *  Created on: Jan 13, 2016
 *      Author: Saman Barghi
 */

#pragma once
#include "generic/IntrusiveContainers.h"
#include "kThread.h"

class uThread;

class ReadyQueue {
    friend class Cluster;
private:
    IntrusiveList<uThread> queue;           //The main producer-consumer queue to keep track of uThreads in the Cluster
    /*
     * One of the goals is to use a LIFO ordering
     * for kThreads, so the queue can self-adjust
     * itself based on the workload
     */
    IntrusiveList<kThread> ktStack;         //an stack to keep track of blocked kThreads on the queue
    std::mutex mtx;
    volatile unsigned int  size;

    void removeMany(IntrusiveList<uThread> &nqueue, mword numkt){
        //TODO: is 1 (fall back to one task per each call) is a good number or should we used size%numkt
        //To avoid emptying the queue and not leaving enough work for other kThreads only move a portion of the queue
        assert(numkt != 0);
        size_t popnum = (size / numkt) ? (size / numkt) : 1; //TODO: This number exponentially decreases, any better way to handle this?

        uThread* ut;
        ut = queue.front();
        nqueue.transferFrom(queue, popnum);
        size -= popnum;
    }

    ReadyQueue() : size(0) {};
    virtual ~ReadyQueue() {};

    inline void unBlock(){
         if(!ktStack.empty()){
            kThread* kt = ktStack.back();
            ktStack.pop_back();
            kt->cv_flag = true;
            kt->cv.notify_one();
        }
    }

    uThread* tryPop() {                     //Try to pop one item, or return null
        uThread* ut = nullptr;
        std::unique_lock<std::mutex> mlock(mtx, std::try_to_lock);              //Do not block on the lock, return immediately to switch to mainUT
        if (mlock.owns_lock() && size != 0) {
            ut = queue.front();
            queue.pop_front();
            size--;
        }
        return ut;
    }

    void tryPopMany(IntrusiveList<uThread> &nqueue, mword numkt) {//Try to pop ReadyQueueSize/#kThreads in cluster from the ready Queue
        std::unique_lock<std::mutex> mlock(mtx, std::try_to_lock);
        if(!mlock.owns_lock() || size == 0) return; // There is no uThreads
        removeMany(nqueue, numkt);
    }

    void popMany(IntrusiveList<uThread> &nqueue, mword numkt) {//Pop with condition variable
        //Spin before blocking
        for (int spin = 1; spin < 52 * 1024; spin++) {
            if (size > 0) break;
            asm volatile("pause");
        }

        std::unique_lock<std::mutex> mlock(mtx);
        //if spin was not enough, simply block
        if (size == 0) {
            //Push the kThread to the stack before waiting on it's cv
            ktStack.push_back(*kThread::currentKT);
            kThread::currentKT->cv_flag = false;                                //Set the cv_flag so we can identify spurious wakeup from notifies
            while (size == 0 || !kThread::currentKT->cv_flag) {kThread::currentKT->cv.wait(mlock);}
        }
        removeMany(nqueue, numkt);
        /*
         * Each kThread unblocks the next kThread in case
         * PushMany was called. First, only one kThread can always hold
         * the lock, so there is no benefit in unblocking all kThreads just
         * for them to be blocked by the mutex.
         * Chaining the unblocking has the benefit of distributing the cost
         * of multiple cv.notify_one() calls over producer + waiting consumers.
         */
        unBlock();
    }

    void push(uThread* ut) {
        std::unique_lock<std::mutex> mlock(mtx);
        queue.push_back(*ut);
        size++;
        unBlock();
    }

    //Push multiple uThreads in the ready Queue
    void pushMany(IntrusiveList<uThread>& utList, size_t count){
        std::unique_lock<std::mutex> mlock(mtx);
        queue.transferFrom(utList, count);
        size+=count;
        unBlock();
    }

    bool empty() const {
        return queue.empty();
    }
};
