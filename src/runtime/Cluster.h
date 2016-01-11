/*
 * Cluster.h
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include <iostream>
#include <condition_variable>
#include <thread>
#include <assert.h>
#include "generic/basics.h"
#include "generic/IntrusiveContainers.h"


class uThread;

class ReadyQueue {
    friend class kThread;
    friend class Cluster;
    friend class LibInitializer;
private:
//	std::priority_queue<uThread*, std::vector<uThread*>, CompareuThread> priorityQueue;
    IntrusiveList<uThread> queue;
    std::mutex mtx;
    std::condition_variable cv;
    volatile unsigned int  size;
    volatile unsigned int waiting;           //number of waiting kThreads

    void removeMany(IntrusiveList<uThread> &nqueue, mword numkt){
        //TODO: is 1 (fall back to one task per each call) is a good number or should we used size%numkt
        //To avoid emptying the queue and not leaving enough work for other kThreads only move a portion of the queue
    	assert(numkt != 0);
        size_t popnum = (size / numkt) ? (size / numkt) : 1;

        uThread* ut;
        ut = queue.front();
        //TODO: for when (size - popnum) < popnum, it's better to traverse the
        //linked list from back instead of front ! call a function to do that.
        queue.transferFrom(nqueue, popnum);
        size -= popnum;
    }
public:
    ReadyQueue() : size(0), waiting(0) {};
    virtual ~ReadyQueue() {};

    uThread* tryPop() {					//Try to pop one item, or return null
        uThread* ut = nullptr;
        std::unique_lock<std::mutex> mlock(mtx, std::try_to_lock);
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
            waiting++;
            while (size == 0) {cv.wait(mlock);}
            waiting--;
        }
        removeMany(nqueue, numkt);
    }

    void push(uThread* ut) {
        std::unique_lock<std::mutex> mlock(mtx);
        queue.push_back(*ut);
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
    void tryGetWorks(IntrusiveList<uThread>&);//Get as many uThreads as possible from the readyQueue and move them to local queue
    void getWork(IntrusiveList<uThread>&);//Get a unit of work or if not available sleep till there is one

    uint64_t getClusterID() const;				//Get the ID of current Cluster

};
