/*
 * Cluster.h
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#ifndef CLUSTER_H_
#define CLUSTER_H_
#include <vector>
#include <mutex>
#include <atomic>
#include <iostream>
#include <condition_variable>
#include <thread>
#include <assert.h>
#include "global.h"
#include "EmbeddedList.h"

class ReadyQueue {
    friend class kThread;
    friend class Cluster;
    friend class LibInitializer;
private:
//	std::priority_queue<uThread*, std::vector<uThread*>, CompareuThread> priorityQueue;
    EmbeddedList<uThread> queue;
    std::mutex mtx;
    std::condition_variable cv;
    volatile mword size;
    volatile mword waiting;           //number of waiting kThreads

    void removeMany(EmbeddedList<uThread> *nqueue, mword numkt){
        //TODO: is 1 (fall back to one task per each call) is a good number or should we used size%numkt
        //To avoid emptying the queue and not leaving enough work for other kThreads only move a portion of the queue
    	assert(numkt != 0);
        mword popnum = (size / numkt) ? (size / numkt) : 1;

        uThread* ut;
        ut = queue.front();
        //TODO: for when (size - popnum) < popnum, it's better to traverse the
        //linked list from back instead of front ! call a function to do that.
        if(size == popnum)
            queue.pop_all();
        else
            queue.pop_many_front(popnum);
        nqueue->push_many_back(ut, popnum);
        size -= popnum;
    }
public:
    ReadyQueue() : size(0) {};
    virtual ~ReadyQueue() {};

    uThread* tryPop() {					//Try to pop one item, or return null
        uThread* ut = nullptr;
        std::unique_lock<std::mutex> mlock(mtx, std::try_to_lock);
        if (mlock.owns_lock() && !queue.empty()) {
            ut = queue.front();
            queue.pop_front();
            size--;
        }
        return ut;
    }

    void tryPopMany(EmbeddedList<uThread> *nqueue, mword numkt) {//Try to pop ReadyQueueSize/#kThreads in cluster from the ready Queue
        std::unique_lock<std::mutex> mlock(mtx, std::try_to_lock);
        if(!mlock.owns_lock() || size == 0) return; // There is no uThreads
        removeMany(nqueue, numkt);
    }

    void popMany(EmbeddedList<uThread> *nqueue, mword numkt) {//Pop with condition variable
        //Spin before blocking
        for (int spin = 1; spin < 52 * 1024; spin++) {
            if (size > 0) break;
            asm volatile("pause");
        }

        std::unique_lock<std::mutex> mlock(mtx);
        //if spin was not enough, simply block
        if (size == 0) {
            waiting++;
            while (queue.empty()) {cv.wait(mlock);}
            waiting--;
        }
        removeMany(nqueue, numkt);
    }

    void push(uThread* ut) {
        std::unique_lock<std::mutex> mlock(mtx);
        queue.push_back(ut);
        size++;
        if (waiting > 0) 		//Signal only when the queue was previously empty
            cv.notify_one();
    }
    bool empty() const {
        return queue.empty();
    }
};

class Cluster {
    friend class kThread;
private:
    ReadyQueue readyQueue;				//There is one ready queue per cluster
    std::atomic_uint  numberOfkThreads;					//Number of kThreads in this Cluster

    static std::atomic_ushort clusterMasterID;				//Global cluster ID holder
    uint64_t clusterID;									//Current Cluster ID

    void initialSynchronization();

public:
    Cluster();
    virtual ~Cluster();

    Cluster(const Cluster&) = delete;
    const Cluster& operator=(const Cluster&) = delete;

    static Cluster* defaultCluster;						//Default cluster
    static Cluster* ioCluster;						    //io cluster
    static std::vector<Cluster*> clusters;				//List of all clusters

    static void invoke(funcvoid1_t, void*) __noreturn;//Function to invoke the run function of a uThread
    void uThreadSchedule(uThread*);	//Put ut in the ready queue to be picked up by kThread
    uThread* tryGetWork();			//Get a unit of work from the ready queue
    void tryGetWorks(EmbeddedList<uThread>*);//Get as many uThreads as possible from the readyQueue and move them to local queue
    void getWork(EmbeddedList<uThread>*);//Get a unit of work or if not available sleep till there is one

    uint64_t getClusterID() const;				//Get the ID of current Cluster

};

#endif /* CLUSTER_H_ */
