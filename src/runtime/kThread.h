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

#ifndef UTHREADS_KTHREADS_H_
#define UTHREADS_KTHREADS_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "../generic/basics.h"
#include "Cluster.h"
#include "uThread.h"

class KTLocal;
class KTVar;

/**
 * @class kThread
 * @brief Object to represent kernel threads
 *
 * kThread is an interface for underlying kernel threads. kThreads
 * are pulling and pushing uThreads from ReadyQueue provided by the Cluster
 * and context switch to them and execute them. Each kThread belongs to
 * only and only one Cluster and it can only pull uThreads from the ReadyQueue
 * of that Cluster. However, kThreads can push uThreads to the ReadyQueue
 * of any cluster.
 *
 * defaultKT is the first kernel thread that executes and is responsible for
 * running the main() function. defaultKT is created when the program starts.
 *
 * Each kThread has a mainUT which is a uThread used when the ReadyQueue
 * is empty. mainUT is used to switch out the previous uThread and either
 * pull uThreads from the ReadyQueue if it's not empty, or block on a
 * condition variable waiting for uThreads to be pushed to the queue.
 *
 * kThreads can be created by passing a Cluster to the constructor of
 * the kThread.
 */
class kThread: public Link<kThread> {
    friend class uThread;
    friend class Cluster;
    friend class IOHandler;
    friend class Scheduler;
private:
    // First 64 bytes (CACHELINE_SIZE)
    /*
     * Pointer to the cluster this kThread
     * belongs to. Each kThread only belongs to
     * one Cluster and can pull uThreads from
     * the ReadyQueue of that Cluster. kThreads
     * can however push uThreads to ReadyQueue
     * of other clusters upon uThread migration.
     */
    Cluster* localCluster; // (8 bytes)

    /*
     * Scheduler object
     */
    Scheduler* scheduler; // (8 bytes)


    KTVar* ktvar;         // (8 bytes)

    /*
     * Pointer to the current running uThread
     */
    uThread* currentUT;   // (8 bytes)

    /*
     * Each kThread has a main uThread that is
     * only used when there is no uThread available
     * on the ReadyQueue. kThread then switches to
     * this uThread and block on the ReadyQueue waiting
     * for more uThreads to arrive.
     */
    uThread* mainUT;    // (8 bytes)

    // First 64 bytes (CACHELINE_SIZE)

     /* Holds the id of the underlying kernel thread */
    std::thread::id threadID;

    /*
     * The actual kernel thread behind this kThread.
     * on Linux it will be a pthread.
     */
    std::thread threadSelf;


    //Only used for defaultKT
    kThread();
    /*
     * Create kThreads that runs a single uThread
     * with the assigned function. These kThreads do
     * not consumer from the ReadyQueu or switch context
     * to another uThread.
     */
    kThread(Cluster&, std::function<void(ptr_t)>, ptr_t);

    /*
     * defaultKT represents the main thread
     * when program starts. This is the thread
     * responsible for running the main function.
     * defaultKT is created at the start of the
     * program and cannot be accessed by user.
     */
    static kThread defaultKT;

    /*
     * Pointer to the current kThread. This thread local variable
     * is used by running uThreads to identify the current
     * kThread that they are being executed over.
     */
    static __thread kThread* currentKT;

    static std::atomic_uint totalNumberofKTs;
    /*
     * This function initializes the required
     * variables for the kThread and then call
     * defaultRun.
     */
    void run();
    /*
     * The main loop of the kThread. This function
     * is only used by mainUT of the kThread. It loops
     * and pulls uThreads from the ReadyQueue or
     * blocks until uThreads are available.
     */
    static void defaultRun(void*) __noreturn;
    /*
     * Same as run() but instead of running the defaultRun
     * run the passed function. It is used to create kThreads
     * that do not get involved with the ReadyQueue such as
     * IOHandler thread.
     */
    void runWithFunc(std::function<void(ptr_t)>, ptr_t);
    //Initialization function for kThread
    void initialize();
    //Initialize mainUT
    void initializeMainUT(bool);
     //Switch the context to the passed uThread.
    void switchContext(uThread*, void* args = nullptr);
    //Pull a uThread from readyQueue and switch the context
    void switchContext(void* args = nullptr);

    /*
     * This function is called after context of the uThread
     * is switched. It is necessary in order to schedule the
     * previous thread or perform the required maintentance
     * before gonig forward.
     */
    static inline void postSwitchFunc(uThread*, void*) __noreturn;

    /*
     * This struct points to an argument and a function. The function
     * is called after suspending a uThread and switching to a new
     * uThread. Two arguments is passed to this function: the old uThread*
     * and the arguments passed by the calling function.
     */
    static __thread funcvoid2_t postSuspendFunc;


    static __thread KTLocal* ktlocal;

    void initialSynchronization();


public:
    //TODO: add a function to create multiple kThreads on a given cluster
    /**
     * @brief Create a kThread on the passed cluster
     * @param The Cluster this kThread belongs to.
     */
    kThread(Cluster&);
    virtual ~kThread();
    void* 	userVar = nullptr;

    ///kThread cannot be copied or assigned.
    kThread(const kThread&) = delete;
    /// @copydoc kThread(const kThread&)
    const kThread& operator=(const kThread&) = delete;

    /**
     * @brief return the native hanlde for the kernel thread
     * @return native handle for the kThread
     *
     * In linux this is pthread_t representation of the thread.
     */
    std::thread::native_handle_type getThreadNativeHandle();
    /**
     * @brief returns the kernel thread ID
     * @return the kThread ID
     *
     * The returned type depends on the platform.
     */
    std::thread::id getID();

    /**
     * @brief Get the pointer to the current kThread
     * @return current kThread
     *
     * This is necessary when a uThread wants to find which
     * kThread it is being executed over.
     */
    static kThread* currentkThread(){return kThread::currentKT;}

    /**
     *
     * @return total number of kThreads running under
     * the program.
     */
    static uint getTotalNumberOfkThreads(){return totalNumberofKTs.load();}
};

#endif /* UTHREADS_KTHREADS_H_ */
