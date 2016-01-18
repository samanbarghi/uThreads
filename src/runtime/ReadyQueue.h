/*
 * ReadyQueue.h
 *
 *  Created on: Jan 13, 2016
 *      Author: Saman Barghi
 */

#ifndef UTHREADS_READY_QUEUE_H_
#define UTHREADS_READY_QUEUE_H_

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


    ReadyQueue() : size(0) {};
    virtual ~ReadyQueue() {};

    ssize_t removeMany(IntrusiveList<uThread> &nqueue){
        //TODO: is 1 (fall back to one task per each call) is a good number or should we used size%numkt
        //To avoid emptying the queue and not leaving enough work for other kThreads only move a portion of the queue
        size_t numkt = kThread::currentKT->localCluster->getNumberOfkThreads();
        assert(numkt != 0);
        size_t popnum = (size / numkt) ? (size / numkt) : 1; //TODO: This number exponentially decreases, any better way to handle this?

        uThread* ut;
        ut = queue.front();
        nqueue.transferFrom(queue, popnum);
        size -= popnum;
        return popnum;
    }

    inline void unBlock(){
         if(!ktStack.empty()){
            kThread* kt = ktStack.back();
            ktStack.pop_back();
            kt->cv_flag = true;
            kt->cv.notify_one();
        }
    }
    /*
     * This function uses std::mutex to exponentially spin
     * to provide a semi-spinlock behavior. If after the
     * first round of exponential spin the lock is not still
     * available, simply block. This is to avoid overhead of
     * blocking, although it adds some overhead to cache misses
     * happening by accessing the mutex memory address.
     * It can also cause out of ordeing accesses to the mutex (not that
     * it was fully FIFO before), which is somehow desired to provide
     * a chance for producers not to be preceded by many consumers that
     * just need to check whether the queue is empty and block on that.
     */
    inline void spinLock(std::unique_lock<std::mutex>& mlock){
        const uint32_t SPIN_START = 4;
        const uint32_t SPIN_END = 4 * 1024;

        size_t spin = SPIN_START;
        for(;;){
            if(mlock.try_lock()) break;
            //exponential spin
            for ( int i = 0; i < spin; i += 1 ) {
                asm volatile( "pause" );
            }
            spin += spin;                   // powers of 2
            if ( spin > SPIN_END ) {
                //spin = SPIN_START;              // prevent overflow
                mlock.lock();
                break;
            }
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

    ssize_t tryPopMany(IntrusiveList<uThread> &nqueue) {//Try to pop ReadyQueueSize/#kThreads in cluster from the ready Queue
        std::unique_lock<std::mutex> mlock(mtx, std::try_to_lock);
        if(!mlock.owns_lock() || size == 0) return -1; // There is no uThreads
        return removeMany(nqueue);
    }

    ssize_t popMany(IntrusiveList<uThread> &nqueue) {//Pop with condition variable
        //Spin before blocking
        for (int spin = 1; spin < 52 * 1024; spin++) {
            if (size > 0) break;
            asm volatile("pause");
        }

        std::unique_lock<std::mutex> mlock(mtx, std::defer_lock);
        spinLock(mlock);
        //if spin was not enough, simply block
        if (fastpath(size == 0)) {
            //Push the kThread to the stack before waiting on it's cv
            ktStack.push_back(*kThread::currentKT);
            kThread::currentKT->cv_flag = false;                                //Set the cv_flag so we can identify spurious wakeup from notifies
            while (size == 0 ) {

                //if the kThread were unblocked by another thread
                //it's been removed from the stack, so put it back on the stack
                if(kThread::currentKT->cv_flag){
                    ktStack.push_back(*kThread::currentKT);
                }
                kThread::currentKT->cv.wait(mlock);}
            //if another thread did not unblock current kThread
            //Remove ourselves from the stack
            if(!kThread::currentKT->cv_flag){
                ktStack.remove(*kThread::currentKT);
            }
        }
        ssize_t res = removeMany(nqueue);
        /*
         * Each kThread unblocks the next kThread in case
         * PushMany was called. First, only one kThread can always hold
         * the lock, so there is no benefit in unblocking all kThreads just
         * for them to be blocked by the mutex.
         * Chaining the unblocking has the benefit of distributing the cost
         * of multiple cv.notify_one() calls over producer + waiting consumers.
         */
        if(size != 0) unBlock();

        return res;
    }

    void push(uThread* ut) {
        std::unique_lock<std::mutex> mlock(mtx, std::defer_lock);
        spinLock(mlock);
        queue.push_back(*ut);
        size++;
        unBlock();
    }

    //Push multiple uThreads in the ready Queue
    void pushMany(IntrusiveList<uThread>& utList, size_t count){
        std::unique_lock<std::mutex> mlock(mtx, std::defer_lock);
        spinLock(mlock);
        queue.transferFrom(utList, count);
        size+=count;
        unBlock();
    }

    bool empty() const {
        return queue.empty();
    }
};

#endif /* UTHREADS_READY_QUEUE_H_ */
