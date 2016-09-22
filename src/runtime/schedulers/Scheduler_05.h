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
#include "../../io/IOHandler.h"
#include <vector>

#define STAGEDEXEC

/*
 * Per uThread variable used by scheduler
 */
struct UTVar{};

/*
 * Local kThread objects related to the
 * scheduler. will be instantiated by static __thread
 */
struct KTLocal{
	static const size_t NUMBEROFSTAGES 	= 1;
    static const size_t UTHREADPERSTAGE 	= 30;

    size_t totalProcessed 			= 0;	//should always be less than UTHREDPERSTAGE
    size_t currentStage 			= 0;	//0 means pop from the ReadyQueue, >0 means pop from one of the local arrays

    IntrusiveStack<uThread> stages[NUMBEROFSTAGES];

};
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

    uThread* stageSwitch(kThread& kt){
    		uThread* ut = nullptr;

//        	std::cout << "Processing Stage: " << kt.ktlocal->currentStage  << ", processed:" << kt.ktlocal->totalProcessed << "Empty Stack:" << (kt.ktlocal->stages[kt.ktlocal->currentStage-1].empty()) << std::endl;

        	while(kt.ktlocal->currentStage != 0){
				if(!(kt.ktlocal->stages[kt.ktlocal->currentStage-1].empty())){
//					std::cout << "Processed" << std::endl;
					ut = kt.ktlocal->stages[kt.ktlocal->currentStage-1].pop();
					--kt.ktlocal->totalProcessed;
					assert(ut != nullptr);
					return ut;
				}else
					kt.ktlocal->currentStage = (++kt.ktlocal->currentStage)%kt.ktlocal->NUMBEROFSTAGES;
        	}
        	//reset totalProcessed since currentStage = 0
			kt.ktlocal->totalProcessed = 0;
        	return ut;
    }

    uThread* nonBlockingSwitch(kThread& kt){
        IOHandler::iohandler.nonblockingPoll();
        uThread* ut = nullptr;

        while( ut == nullptr){
        	if(kt.ktlocal->currentStage > 0 ) ut = stageSwitch(kt);
        	else {
        		ut = runQueue.pop();

				if(ut == nullptr){
//					std::cout << "Here" << std::endl;
					//if ( kt.currentUT->state == uThread::State::YIELD)
					//	return nullptr; //if the running uThread yielded, continue running it
					if(!kt.ktlocal->stages[0].empty()){
//						std::cout << "And Here" << std::endl;
						kt.ktlocal->currentStage++;
					}else
						ut = kt.mainUT;
				}else{
					if(++kt.ktlocal->totalProcessed >= kt.ktlocal->UTHREADPERSTAGE )
					{
						if(kt.ktlocal->stages[0].empty())
							kt.ktlocal->totalProcessed = 0;
						else
							kt.ktlocal->currentStage++;
					}
				}
        	}
//        	std::cout << "Stage: " << kt.ktlocal->currentStage  << ", processed:" << kt.ktlocal->totalProcessed << std::endl;
        }


        assert(ut != nullptr);
        return ut;
    }

    uThread* blockingSwitch(kThread& kt){

        /* before blocking inform the poller thread of our
         * intent.
         */
        IOHandler::iohandler.sem.post();
        uThread* ut = nullptr;

        while(ut == nullptr){
			sem.wait();
			ut = runQueue.pop();
        }
        assert(ut != nullptr);

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
