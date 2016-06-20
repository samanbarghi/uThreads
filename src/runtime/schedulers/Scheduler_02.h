/*
 * Scheduler_02.h
 *
 *  Created on: May 4, 2016
 *      Author: Saman Barghi
 */

#ifndef SRC_RUNTIME_SCHEDULERS_SCHEDULER_02_H_
#define SRC_RUNTIME_SCHEDULERS_SCHEDULER_02_H_
#include "../../generic/IntrusiveContainers.h"
#include "../../generic/Semaphore.h"
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

    semaphore sem;
    BlockingMPSCQueue<uThread> runQueue;
    /* ************** Scheduling wrappers *************/
    //Schedule a uThread on a cluster
    static void schedule(uThread* ut, kThread& kt){
        assert(ut != nullptr);
        if(kt.scheduler->runQueue.push(*ut))
            kt.scheduler->sem.post();
    }
    //Put uThread in the ready queue to be picked up by related kThreads
    void schedule(uThread* ut) {
        assert(ut != nullptr);
        if(runQueue.push(*ut))
            sem.post();
    }

    //Schedule many uThreads
    void schedule(IntrusiveQueue<uThread>& queue, size_t count) {
        assert(!queue.empty());
        uThread* first = queue.front();
        uThread* last = queue.removeAll();
        if(runQueue.push(*first, *last))
            sem.post();
    }

    uThread* nonBlockingSwitch(kThread& kt){
        uThread* ut = runQueue.pop();
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

        sem.wait();
        uThread* ut = runQueue.pop();
        assert(ut != nullptr);

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

    static void bulkPush(){
        for (auto& x: Cluster::clusterList){
            bulkPush(*x);
        }
    }

    static void bulkPush(Cluster &cluster){
        for (auto& x: cluster.ktVector){
            if(x->ktvar->bulkCounter > 0)
                x->scheduler->schedule(x->ktvar->bulkQueue, x->ktvar->bulkCounter);
                x->ktvar->bulkCounter = 0;
        }
    }
};

#endif /* SRC_RUNTIME_SCHEDULERS_SCHEDULER_02_H_ */
