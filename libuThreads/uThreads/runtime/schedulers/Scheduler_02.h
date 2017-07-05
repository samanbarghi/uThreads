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

#ifndef SRC_RUNTIME_SCHEDULERS_SCHEDULER_02_H_
#define SRC_RUNTIME_SCHEDULERS_SCHEDULER_02_H_

#include "../../generic/IntrusiveContainers.h"
#include "../../generic/Semaphore.h"
#include "../../io/IOHandler.h"
#include "../kThread.h"


namespace uThreads {
namespace runtime {
using uThreads::io::IOHandler;
using uThreads::generic::BlockingMPSCQueue;
using uThreads::generic::IntrusiveQueue;
/*
 * Per uThread variable used by scheduler
 */
struct UTVar {
};

/*
 * Local kThread objects related to the
 * scheduler. will be instantiated by static __thread
 */
struct KTLocal {
};

/*
 * Per kThread variable related to the scheduler
 */
struct KTVar {
    /*
     * Count the number of items in bulkQueue
     */
    size_t bulkCounter;

    /*
     * This list is used to schedule uThreads in bulk.
     * For now it is only used in IOHandler
     */
    IntrusiveQueue<uThread> bulkQueue;

    KTVar() : bulkCounter(0) {}
};

struct ClusterVar {
};

class Scheduler {
    friend class uThread;

    friend class kThread;

    friend class Cluster;

    friend class uThreads::io::IOHandler;


 private:

    uThreads::generic::semaphore sem;
    BlockingMPSCQueue<uThread> runQueue;
    /* ************** Scheduling wrappers *************/
    // Schedule a uThread on a cluster
    static void schedule(uThread *ut, const kThread &kt) {
        assert(ut != nullptr);
        if (kt.scheduler->runQueue.push(*ut))
            kt.scheduler->sem.post();
    }

    // Put uThread in the ready queue to be picked up by related kThreads
    void schedule(uThread *ut) {
        assert(ut != nullptr);
        if (runQueue.push(*ut))
            sem.post();
    }

    // Schedule many uThreads
    void schedule(IntrusiveQueue<uThread> &queue, size_t count) {
        assert(!queue.empty());
        uThread *first = queue.front();
        uThread *last = queue.removeAll();
        if (runQueue.push(*first, *last))
            sem.post();
    }

    uThread *nonBlockingSwitch(const kThread &kt) {
        kt.iohandler->nonblockingPoll();
        uThread *ut = runQueue.pop();
        if (ut == nullptr) {
            // if the running uThread yielded, continue running it
            if (kt.currentUT->state == uThread::State::YIELD)
                return nullptr;
            ut = kt.mainUT;
        }
        assert(ut != nullptr);
        return ut;
    }

    uThread *blockingSwitch(const kThread &kt) {

        /* before blocking inform the poller thread of our
         * intent.
         */
        kt.iohandler->sem.post();

        uThread *ut = nullptr;
        while (ut == nullptr) {
            sem.wait();
            ut = runQueue.pop();
        }
        assert(ut != nullptr);

        /*
         * We signaled the poller thread, now it's the time
         * to signal it again that we are unblocked.
         */
        while (!kt.iohandler->sem.trywait()) {}

        return ut;
    }

    /* assign a scheduler to a kThread */
    static Scheduler *getScheduler(const Cluster &cluster) {
        return new Scheduler();
    }

    static void prepareBulkPush(uThread *ut) {
        assert(ut->homekThread != nullptr);
        ut->homekThread->ktvar->bulkQueue.push(*ut);
        ut->homekThread->ktvar->bulkCounter++;
    }

    static void bulkPush() {
        for (auto &x : Cluster::clusterList) {
            bulkPush(*x);
        }
    }

    static void bulkPush(const Cluster &cluster) {
        for (auto &x : cluster.ktVector) {
            if (x->ktvar->bulkCounter > 0)
                x->scheduler->schedule(x->ktvar->bulkQueue,
                                       x->ktvar->bulkCounter);
            x->ktvar->bulkCounter = 0;
        }
    }

    static void bulkPush(const kThread &kt) {
        kt.scheduler->schedule(kt.ktvar->bulkQueue, kt.ktvar->bulkCounter);
    }
};
}  // namespace runtime
}  // namespace uThreads
#endif /* SRC_RUNTIME_SCHEDULERS_SCHEDULER_02_H_ */
