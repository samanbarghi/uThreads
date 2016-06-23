/*
 * Scheduler_03.h
 *
 *  Created on: Jun 2, 2016
 *      Author: saman
 */

#ifndef SRC_RUNTIME_SCHEDULERS_SCHEDULER_03_H_
#define SRC_RUNTIME_SCHEDULERS_SCHEDULER_03_H_

#include "../../generic/IntrusiveContainers.h"
#include "../kThread.h"
#include "io/IOHandler.h"

/*
 * Per uThread variable used by scheduler
 */
struct UTVar{};

/*
 * Local kThread objects related to the
 * scheduler. will be instantiated by static __thread
 */
struct KTLocal{};
/*
 * Per kThread variable related to the scheduler
 */
struct KTVar{
    /*
     * Count the number of items in bulkQueue
     */
    size_t bulkCounter;

    /*
     * This list is used to schedule uThreads in bulk.
     * For now it is only used in IOHandler
     */
    IntrusiveQueue<uThread> bulkQueue;

    KTVar(): bulkCounter(0){};
};

struct ClusterVar{};

class Scheduler {
    friend class kThread;
    friend class Cluster;
    friend class IOHandler;
    friend class uThread;
private:
    IntrusiveQueue<uThread> runQueue;

    std::mutex mtx;
    std::condition_variable cv;


    uThread* __tryPop(){

       std::unique_lock<std::mutex> ul(mtx, std::try_to_lock);
       if(ul.owns_lock() && !runQueue.empty()){
           uThread* ut = runQueue.front();
           runQueue.pop();
           return ut;
       }
       return nullptr;
    }

    uThread* __Pop(){
        std::unique_lock<std::mutex> ul(mtx);
        while(runQueue.empty())
            cv.wait(ul);

        uThread* ut = runQueue.front();
        runQueue.pop();
        return ut;
    }
    /* ************** Scheduling wrappers *************/

    //Schedule a uThread on a cluster
    static void schedule(uThread* ut, kThread& kt){
        assert(ut != nullptr);
        std::unique_lock<std::mutex> ul(kt.scheduler->mtx);
        kt.scheduler->runQueue.push(*ut);
        kt.scheduler->cv.notify_one();
    }
    //Put uThread in the ready queue to be picked up by related kThreads
    void schedule(uThread* ut) {
        assert(ut != nullptr);
        std::unique_lock<std::mutex> ul(mtx);
        runQueue.push(*ut);
        cv.notify_one();
    }

    //Schedule many uThreads
    void schedule(IntrusiveQueue<uThread>& queue, size_t count) {
        assert(!queue.empty());
        std::unique_lock<std::mutex> ul(mtx);
        runQueue.transferAllFrom(queue);
        cv.notify_one();
    }

    uThread* nonBlockingSwitch(kThread& kt){
        IOHandler::iohandler.nonblockingPoll();
        uThread* ut = __tryPop();
        if(ut == nullptr){
            if ( kt.currentUT->state == uThread::State::YIELD)
                return nullptr; //if the running uThread yielded, continue running it
            ut = kt.mainUT;
        }
        assert(ut != nullptr);
        return ut;
    }

    uThread* blockingSwitch(kThread& kt){
        /* before blocking inform the poller thread of our
         * intent.
         */
        IOHandler::iohandler.sem.post();

        uThread* ut = __Pop();

        /*
         * We signaled the poller thread, now it's the time
         * to signal it again that we are unblocked.
         */
        while(!IOHandler::iohandler.sem.trywait());

        return ut;
    }

    /* assign a scheduler to a kThread */
    static Scheduler* getScheduler(Cluster& cluster){
        return new Scheduler();
    }

    static void prepareBulkPush(uThread* ut){
        assert(ut->homekThread != nullptr);
        ut->homekThread->ktvar->bulkQueue.push(*ut);
        ut->homekThread->ktvar->bulkCounter++;
    }

    static void bulkPush(Cluster &cluster){
        for (auto& x: cluster.ktVector){
            if(x->ktvar->bulkCounter > 0)
                x->scheduler->schedule(x->ktvar->bulkQueue, x->ktvar->bulkCounter);
                x->ktvar->bulkCounter = 0;
        }
    }

};
#endif /* SRC_RUNTIME_SCHEDULERS_SCHEDULER_03_H_ */
