/* *****************************************************************************
 *     Copyright © 2015, 2016 Saman Barghi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

#ifndef UTHREADS_MOODY_CAMEL_READY_QUEUE_H_
#define UTHREADS_MOODY_CAMEL_READY_QUEUE_H_

#include "generic/IntrusiveContainers.h"
#include "generic/blockingconcurrentqueue.h"
#include "generic/ArrayQueue.h"
#include "kThread.h"

class uThread;
class IOHandler;

class MCReadyQueue {
    friend class Cluster;
    friend class kThread;
    friend class IOHandler;
private:
    /*
     * Start and end points for the exponential spin
     */
    const uint32_t SPIN_START = 4;
    const uint32_t SPIN_END = 4 * 1024;

    std::mutex mtx;//mutex to protect the queue
    volatile unsigned int  size;//keep track of the size of the queue
    /*
     * The main producer-consumer queue
     * to keep track of uThreads in the Cluster
     */
    moodycamel::BlockingConcurrentQueue<uThread*> queue;
    /*
     * The goal is to use a LIFO ordering
     * for kThreads, so the queue can self-adjust
     * itself based on the workload. Thus, a stack
     * is used to keep track of kThreads blocked on
     * an empty queue.
     */
    IntrusiveList<kThread> ktStack;
    /*
     * This variable is used to keep
     * a history of the latest number
     * of uThreads popped from the queue.
     * This will be used to calculate the
     * new number.
     */
    size_t lastPopNum;

    MCReadyQueue() : size(0), lastPopNum(0) {};
    virtual ~MCReadyQueue() {};

    /*
     * Remove multiple uThreads from the queue
     * all at once. 'popnum' is calcluated
     * everytime based on the last popnum, size
     * of the queue and number of kThreads in
     * the cluster.
     * All uThreads are directly moved to the
     * local queue of the kThread (nqueue) passed
     * to the function in bulk. This saves a lot
     * of overhead.
     */
    ssize_t removeMany(IntrusiveList<uThread> &nqueue){
 /*       //TODO: is 1 (fall back to one task per each call) is a good number or should we used size%numkt
        //To avoid emptying the queue and not leaving enough work for other kThreads only move a portion of the queue
        size_t numkt = kThread::currentKT->localCluster->getNumberOfkThreads();
        size_t popnum = (size / numkt) ? (size / numkt) : 1;
        popnum = (popnum < lastPopNum)? lastPopNum : popnum;
        popnum = (popnum > size)      ? size       : popnum;
        lastPopNum = popnum;

        uThread* ut;
        ut = queue.front();
        nqueue.transferFrom(queue, popnum);
        size -= popnum;
        return popnum;*/
        return 0;
    }

    /*
     * Unblock a kThread that is waiting for
     * uThreads to arrive. Each kThread has its
     * own Condition Variable, and since kThreads
     * are lining up in LIFO order in ktStack, thus
     * this code pops a kThread and unblock it through
     * sending a notificiation on its CV.
     */
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
    /*
     * Try popping one item and do not block.
     * give up immediately if cannot acquire
     * the mutex or the queue is empty.
     */

    uThread* tryPop() {                     //Try to pop one item, or return null
        uThread* ut = nullptr;
        if(!queue.try_dequeue(*kThread::ctok, ut)) ut = nullptr;
        return ut;
    }

    /*
     * Try popping multiple items and do not block.
     * give up immediately if cannot acquire
     * the mutex or the queue is empty.
     */
    ssize_t tryPopMany() {
        int size = queue.size_approx();
        size_t numkt = kThread::currentKT->localCluster->getNumberOfkThreads();
        size_t popnum = (size/ numkt) ? (size / numkt) : 1;
        if(popnum > kThread::localQueue->size()) popnum = kThread::localQueue->size();

        return queue.try_dequeue_bulk(*kThread::ctok, kThread::localQueue->begin(), popnum);
    }

    /*
     * Pop multiple items from the queue and block
     * if the queue is empty.
     */
    ssize_t popMany() {//Pop with condition variable
        int size = queue.size_approx();
        size_t numkt = kThread::currentKT->localCluster->getNumberOfkThreads();
        size_t popnum = (size/ numkt) ? (size / numkt) : 1;
        if(popnum > kThread::localQueue->size()) popnum = kThread::localQueue->size();

        return queue.wait_dequeue_bulk(*kThread::ctok, kThread::localQueue->begin(), popnum);
    }

    /*
     * Push a single uThread to the queue and
     * wake one kThread up.
     */
    void push(uThread* ut) {
        queue.enqueue(std::move(ut));
    }

    /*
     * Push multiple uThreads to the queue, and wake one
     * kThread up. That kThread then wakes up another kThread
     * if there are still uThreads left in the queue.
     * uThreads are copied directly from the passed container
     * to the queue in bulk to avoid the overhead.
     */
    void pushMany(moodycamel::ProducerToken& ptok, ArrayQueue<uThread, EPOLL_MAX_EVENT>& nqueue){
        queue.enqueue_bulk(ptok, nqueue.begin(), nqueue.count());
    }

    /*
     * Whether the queue is empty or not.
     */
    bool empty() const {
        return queue.size_approx() != 0;
    }
};

#endif /* UTHREADS_MOODY_CAMEL_READY_QUEUE_H_ */
