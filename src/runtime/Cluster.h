/*
 * Cluster.h
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#ifndef UTHREADS_CLUSTER_H_
#define UTHREADS_CLUSTER_H_

#include <mutex>
#include <atomic>
#include <iostream>
#include <condition_variable>
#include <thread>
#include <assert.h>
#include "generic/basics.h"
#include "generic/IntrusiveContainers.h"

class uThread;
class ReadyQueue;

class Cluster {
    friend class kThread;
    friend class uThread;
private:
    ReadyQueue* readyQueue;                              //There is one ready queue per cluster
    std::atomic_uint  numberOfkThreads;                 //Number of kThreads in this Cluster

    static std::atomic_ushort clusterMasterID;          //Global cluster ID holder
    uint64_t clusterID;                                 //Current Cluster ID

    void initialSynchronization();

    void schedule(uThread*);                                    //Put uThread in the ready queue to be picked up by related kThreads
    void scheduleMany(IntrusiveList<uThread>&, size_t );        //Schedule many uThreads
    ssize_t getWork(IntrusiveList<uThread>&);                   //Get a unit of work or if not available sleep till there is one
    uThread* tryGetWork();                                      //Get a unit of work from the ready queue
    ssize_t tryGetWorks(IntrusiveList<uThread>&);               //Get as many uThreads as possible from the readyQueue and move them to local queue


public:
    Cluster();
    virtual ~Cluster();

    Cluster(const Cluster&) = delete;
    const Cluster& operator=(const Cluster&) = delete;

    static Cluster defaultCluster;                                          //Default cluster
    static void invoke(funcvoid3_t, ptr_t, ptr_t, ptr_t) __noreturn;        //Function to invoke the run function of a uThread

    uint64_t getClusterID() const {return clusterID;};                      //Get the ID of current Cluster
    size_t  getNumberOfkThreads() const { return numberOfkThreads.load();};

};

#endif /* UTHREADS_CLUSTER_H_ */
