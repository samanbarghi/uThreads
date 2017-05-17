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
#include <vector>
#include <assert.h>
#include "../generic/basics.h"
#include "../generic/IntrusiveContainers.h"


namespace uThreads {
namespace io {
class IOHandler;
}  // namespace io
namespace runtime {
class uThread;

class Scheduler;

class kThread;

class ClusterVar;

/**
 * @class Cluster
 * @brief Scheduler and Cluster of kThreads.
 *
 * Cluster is an entity that contains multiple kernel threads (kThread).
 * Each cluster is responsible for maintaining a ready queue
 * and performing basic scheduling tasks. Programs can have as many Clusters
 * as is necessary.
 * The Cluster's ReadyQueue is a multiple-producer multiple-consumer queue where
 * consumers are only kThreads belonging to that Cluster, and producers can be any
 * running kThread. kThreads constantly push and pull uThreads to/from the ReadyQueue.
 * Cluster is an interface between kThreads and the ReadyQueue,
 * and also provides the means to group kThreads together.
 *
 * Each Cluster has its own IOHandler. IOHandler is responsible for
 * providing asynchronous nonblocking access to IO devices. For now each
 * instance of an IOHandler has its own dedicated poller thread, which
 * means each cluster has a dedicated IO poller thread when it is created.
 * This might change in the future.
 * Each uThread that requires access to IO uses the IOHandler to avoid blocking
 * the kThread, if the device is ready for read or write, the uThread continues
 * otherwise it blocks until it is ready, and the kThread execute another uThread
 * from the ReadyQueue.
 *
 * When the program starts a defaultCluster is created for the kernel thread that
 * runs the _main_ function. defaultCluster can be used like any other clusters.
 */
class Cluster {
    friend class kThread;

    friend class uThread;

    friend class Connection;

    friend class uThreads::io::IOHandler;

    friend class Scheduler;

 private:
    // First 64 bytes (CACHELINE_SIZE)
    Scheduler *scheduler;                               // (8 bytes)

    uThreads::io::IOHandler *iohandler;                  // (8 bytes)

    /*
     * Scheduler's defined cluster variables
     */
    ClusterVar *clustervar;                             // (8 bytes)

    // First 64 bytes (CACHELINE_SIZE)

    static std::vector<Cluster *> clusterList;

    /*
     * last kThread assigned to a uThread,
     * for now kThread assignment is round robin
     * manner.
     */
    std::atomic<size_t> ktLast;

    std::atomic_uint numberOfkThreads;          // Number of kThreads in this Cluster
    /*
     * Vector of kThreads in this Cluster
     */
    std::vector<kThread *> ktVector;

    uint64_t clusterID;                         // Current Cluster ID

    std::mutex mtx;                             // Mutex used for initializations

    static std::atomic_ushort clusterMasterID;  // Global cluster ID holder



    /**
     * @brief defaultCluster includes the main thread.
     * This cluster is created before the program starts.
     * The first kernel thread that runs the main function
     * (defaultKT) belongs to this cluster. It is static
     * and can be reached either through Cluster::defaultCluster
     * or getDefaultCluster() function.
     */
    static Cluster defaultCluster;

    void initialSynchronization();

    /*
     * Add a new kThread to this Cluster
     */
    void addNewkThread(kThread &);

    /*
     * Assign a kThread to the requesting uThread
     */
    kThread *assignkThread();

 public:
    /**
     * Create a new Cluster
     */
    Cluster();

    virtual ~Cluster();

    /**
     * Cluster cannot be copied or assigned.
     */
    Cluster(const Cluster &) = delete;

    /// @copydoc Cluster(const Cluster&)
    const Cluster &operator=(const Cluster &) = delete;

    /**
      * @copybrief Cluster::defaultCluster
      * @return defaultCluster
      *
      * @copydetails defaultCluster
      */
    static Cluster &getDefaultCluster() {
        return defaultCluster;
    }

    /**
     * @brief Get the ID of Cluster
     * @return The ID of the cluster
     *
     */
    uint64_t getID() const {
        return clusterID;
    }

    /**
     * @brief Total number of kThreads belonging to this cluster
     * @return Total number of kThreads belonging to this cluster
     */
    size_t getNumberOfkThreads() const {
        return numberOfkThreads.load();
    }
};
}  // namespace runtime
}  // namespace uThreads
#endif /* UTHREADS_CLUSTER_H_ */
