/*******************************************************************************
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

#ifndef SRC_RUNTIME_SCHEDULERS_SCHEDULER_01_H_
#define SRC_RUNTIME_SCHEDULERS_SCHEDULER_01_H_
#include "../../generic/IntrusiveContainers.h"
#include "../Cluster.h"
#include "../kThread.h"

class IOHandler;

/*
 * Local kThread objects related to the
 * scheduler. will be instantiated by static __thread
 */
struct KTLocal{
    /*
     * local readyQueue for kThread which is only
     * accessible by this kThread (thus thread local).
     * This is used to bulk pull uThreads from the central
     * ReadyQueue. The bulk operation lowers the overhead
     * of pulling threads and accessing the central ReadyQueue
     * as the central ReadyQueue i protected by a mutex.
     */
    IntrusiveQueue<uThread> lrq;
};
/*
 * Per kThread variable related to the scheduler
 */
struct KTVar{
    /*
     *  Condition variable to be used by Cluster's
     *  ReadyQueue. Each kThread provides its own CV
     *  in order to provide a LIFO blocking order.
     */
    std::condition_variable cv;
    /*
     * cv_flag is used to detect spurious
     * wake ups.
     */
    bool cv_flag;
    KTVar() : cv_flag(false){};
};

/*
 * Cluster variables
 */
struct ClusterVar{
    /*
     * This list is used to schedule uThreads in bulk.
     * For now it is only used in IOHandler
     */
    IntrusiveQueue<uThread> bulkQueue;

    /*
     * Count the number of items in bulkQueue
     */
    size_t bulkCounter;
};
class Scheduler {
    friend class kThread;
    friend class Cluster;
    friend class IOHandler;
    friend class uThread;
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
    IntrusiveQueue<uThread> queue;
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

    Scheduler() : size(0), lastPopNum(0) {};
    virtual ~Scheduler() {};

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
    ssize_t __removeMany(IntrusiveQueue<uThread> &nqueue){
        //TODO: is 1 (fall back to one task per each call) is a good number or should we used size%numkt
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
        return popnum;
    }

    /*
     * Unblock a kThread that is waiting for
     * uThreads to arrive. Each kThread has its
     * own Condition Variable, and since kThreads
     * are lining up in LIFO order in ktStack, thus
     * this code pops a kThread and unblock it through
     * sending a notificiation on its CV.
     */
    inline void __unBlock(){
         if(!ktStack.empty()){
            kThread* kt = ktStack.back();
            ktStack.pop_back();
            kt->ktvar->cv_flag = true;
            kt->ktvar->cv.notify_one();
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
    inline void __spinLock(std::unique_lock<std::mutex>& mlock){
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

    uThread* __tryPop() {                     //Try to pop one item, or return null
        uThread* ut = nullptr;
        /*
         * Do not block on the lock,
         * return immediately.
         */
        std::unique_lock<std::mutex> mlock(mtx, std::try_to_lock);
        // no uThreads, or could not acquire the lock
        if (mlock.owns_lock() && size != 0) {
            ut = queue.front();
            queue.pop();
            size--;
        }
        return ut;
    }

    /*
     * Try popping multiple items and do not block.
     * give up immediately if cannot acquire
     * the mutex or the queue is empty.
     */
    ssize_t __tryPopMany(IntrusiveQueue<uThread> &nqueue) {
        std::unique_lock<std::mutex> mlock(mtx, std::try_to_lock);
        if(!mlock.owns_lock()) return -1;
        if(size == 0) return 0;
        return __removeMany(nqueue);
    }

    /*
     * Pop multiple items from the queue and block
     * if the queue is empty.
     */
    ssize_t __popMany(IntrusiveQueue<uThread> &nqueue) {//Pop with condition variable
        std::unique_lock<std::mutex> mlock(mtx, std::defer_lock);
        __spinLock(mlock);
        if (fastpath(size == 0)) {
            //Push the kThread to stack before waiting on it's cv
            ktStack.push_back(*kThread::currentKT);
            /*
             * Set the cv_flag so we can identify
             * spurious wake ups from notifies
             */
            kThread::currentKT->ktvar->cv_flag = false;
            while (size == 0 ) {
                /*
                 * cv_flag can be true if the kThread were unblocked
                 * by another thread but by the time it acquires the
                 * mutex the queue is empty again. If so the kThread
                 * has been removed from the stack, thus put it back
                 * on the stack
                 */
                if(kThread::currentKT->ktvar->cv_flag){
                    ktStack.push_back(*kThread::currentKT);
                }
                kThread::currentKT->ktvar->cv.wait(mlock);
            }
            /*
             * If cv_flag is not true, it means the kThread
             * experienced a spurious wake up and thus
             * has not been removed from the stack. Since
             * ktStack is intrusive the kThread can remove
             * itself from the stack.
             */
            if(!kThread::currentKT->ktvar->cv_flag){
                ktStack.remove(*kThread::currentKT);
            }
        }
        ssize_t res = __removeMany(nqueue);
        /*
         * Each kThread unblocks the next kThread in case
         * PushMany was called. Only one kThread can always hold
         * the lock, so there is no benefit in unblocking all kThreads just
         * for them to be blocked by the mutex.
         * Chaining the unblocking has the benefit of distributing the cost
         * of multiple cv.notify_one() calls over producer + waiting consumers.
         */
        if(size != 0) __unBlock();

        return res;
    }

    /*
     * Push a single uThread to the queue and
     * wake one kThread up.
     */
    void __push(uThread* ut) {
        std::unique_lock<std::mutex> mlock(mtx, std::defer_lock);
        __spinLock(mlock);
        queue.push(*ut);
        size++;
        __unBlock();
    }

    /*
     * Push multiple uThreads to the queue, and wake one
     * kThread up. That kThread then wakes up another kThread
     * if there are still uThreads left in the queue.
     * uThreads are copied directly from the passed container
     * to the queue in bulk to avoid the overhead.
     */
    void __push(IntrusiveQueue<uThread>& utList, size_t count){
        std::unique_lock<std::mutex> mlock(mtx, std::defer_lock);
        __spinLock(mlock);
        queue.transferAllFrom(utList);
        size+=count;
        __unBlock();
    }

    /*
     * Whether the queue is empty or not.
     */
    bool __empty() const {
        return queue.empty();
    }

    /* ************** Scheduling wrappers *************/
    //Schedule a uThread on a cluster
    static void schedule(uThread* ut, kThread& kt){
        assert(ut != nullptr);
        kt.scheduler->__push(ut);
    }
    //Put uThread in the ready queue to be picked up by related kThreads
    void schedule(uThread* ut) {
        assert(ut != nullptr);
        //Scheduling uThread
        __push(ut);
    }

    //Schedule many uThreads
    void schedule(IntrusiveQueue<uThread>& queue, size_t count) {
        assert(!queue.empty());
        __push(queue, count);
    }

    uThread* nonBlockingSwitch(kThread& kt){
        uThread* ut = nullptr; /*  First check the local queue */
        IntrusiveQueue<uThread>& ktrq = kt.ktlocal->lrq;
        if (!ktrq.empty()) {   //If not empty, grab a uThread and run it
            ut = ktrq.front();
            ktrq.pop();
        } else {                //If empty try to fill

            ssize_t res = __tryPopMany(ktrq);   //Try to fill the local queue
            if (res > 0) {       //If there is more work start using it
                ut = ktrq.front();
                ktrq.pop();
            } else {        //If no work is available, Switch to defaultUt
                if (res == 0 && kt.currentUT->state == uThread::State::YIELD)
                    return nullptr; //if the running uThread yielded, continue running it
                ut = kt.mainUT;
            }
        }
        assert(ut != nullptr);
        return ut;
    }

    uThread* blockingSwitch(kThread& kt){
        ssize_t res = __popMany(kt.ktlocal->lrq);
        if(res ==0) return nullptr;
        //ktReadyQueue should not be empty at this point
        assert(!kt.ktlocal->lrq.empty());
        uThread* ut = kt.ktlocal->lrq.front();
        kt.ktlocal->lrq.pop();
        return ut;
    }

    /* assign a scheduler to a kThread */
    static Scheduler* getScheduler(Cluster& cluster){
       if(slowpath(cluster.scheduler == nullptr)){
           std::unique_lock<std::mutex> ul(cluster.mtx);
           if(cluster.scheduler == nullptr)
               cluster.scheduler = new Scheduler();
       }
       return cluster.scheduler;
    }

    static void prepareBulkPush(uThread* ut){
        ut->currentCluster->clustervar->bulkQueue.push(*ut);
        ut->currentCluster->clustervar->bulkCounter++;
    }

    static void bulkPush(Cluster &cluster){
        cluster.scheduler->schedule(cluster.clustervar->bulkQueue, cluster.clustervar->bulkCounter);
        cluster.clustervar->bulkCounter =0;
    }
};
#endif /* SRC_RUNTIME_SCHEDULER_01_H_ */
