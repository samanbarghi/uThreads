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

#ifndef UTHREADS_CLUSTER_H_
#define UTHREADS_CLUSTER_H_

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <assert.h>
#include "generic/basics.h"
#include "generic/IntrusiveContainers.h"

class uThread;
class ReadyQueue;
class IOHandler;

class Cluster {
    friend class kThread;
    friend class uThread;
    friend class Connection;
    friend class IOHandler;
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

    IOHandler* iohandler;
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
