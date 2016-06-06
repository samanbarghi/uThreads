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

#ifndef UTHREADS_UTHREAD_H_
#define UTHREADS_UTHREAD_H_

#include <mutex>
#include <atomic>
#include "generic/basics.h"
#include "generic/IntrusiveContainers.h"
#include "Stack.h"
#include "BlockingSync.h"

class IOHandler;
class Cluster;
class uThreadCache;
class Scheduler;
class kThread;
class UTVar;

/**
 * @class uThread
 * @brief user-level threads (fiber)
 *
 * uThreads are building blocks of this library. They are lightweight
 * threads and do not have the same context switch overhead as kernel threads.
 * Each uThread is an execution unit provided to run small tasks. uThreads are
 * being managed cooperatively and there is no preemption involved. uThreads
 * either yield, migrate, or blocked and giving way to other uThreads to get a
 * chance to run.
 *
 * Due to the cooperative nature of uThreads, it is recommended that
 * uThreads do not block the underlying kernel thread for a long time. However,
 * since there can be multiple kernel threads (kThread) in the program, if one
 * or more uThreads block underlying kThreads for small amount of time the
 * execution of the program does not stop and other kThreads keep executing.
 *
 * Another pitfall can be when all uThreads are blocked and each waiting for an
 * event to occurs which can cause deadlocks. It is programmer's responsibility
 * to make sure this never happens. Although it never happens unless a uThread
 * is blocked without a reason (not waiting on a lock or IO), otherwise there is
 * always an event (another uThread or polling structure) that unblock the
 * uThread.
 *
 * Each uThread has its own stack which is very smaller than kernel thread's
 * stack. This stack is allocated when uThread is created.
 */

class uThread: public Link<uThread> {
    friend class uThreadCache;
    friend class kThread;
    friend class Cluster;
    friend class BlockingQueue;
    friend class IOHandler;
    friend class Scheduler;

    // First 64 bytes (CACHELINE_SIZE)
    //Link* prev                        (8 bytes)
    //Link* next                        (8 bytes)
protected:
    /*
     * Thread variables
     */

    /*
     * Current Cluster that uThread is executed on.
     * This variable is used for migrating to another Cluster.
     */
    Cluster* currentCluster;          //(8 bytes)

    /*
     * Current kThread assigned to this uThread
     */
    kThread* homekThread;             //(8 bytes)

    /*
     * Stack Boundary
     */
    vaddr stackPointer;         // holds stack pointer while thread inactive (8 bytes)
    vaddr stackBottom;          //Bottom of the stack                        (8 bytes)
    size_t stackSize;           //Size of the stack of the current uThread   (8 bytes ?)

    UTVar*  utvar;
    // First 64 bytes (CACHELINE_SIZE)
    uint64_t uThreadID;                         //unique Id for this uThread (8 bytes)
private:

    /*
     * initUT is the initial uThread that is created when program starts.
     * initUT holds the context that runs the main thread. It does not have
     * a separate stack and takes over the underlying kThread(defaultKT)
     * stack to be able to take control of the main function.
     */
    static uThread* initUT;


    /* This is only used to create mainUT for kThreads */
   static uThread* createMainUT(Cluster&);

   /* Variables for Joinable uThread */
   Mutex   joinMtx;
   ConditionVariable joinWait;

   /*
    * Wait or Signal the other thread
    */
   inline void waitOrSignal(){
       joinMtx.acquire();
       if(joinWait.empty()){
           joinWait.wait(joinMtx);
           joinMtx.release();
       }else{
           joinWait.signal(joinMtx);
       }
   }


protected:
     /* hold various states that uThread can go through */
    enum class State : std::uint8_t {
        INITIALIZED,                                    //uThread is initialized
        READY,                                      //uThread is in a ReadyQueue
        RUNNING,                                           //<uThread is Running
        YIELD,                                             //uThread is Yielding
        MIGRATE,                                  //Migrating to another cluster
        WAITING,                                            //uThread is Blocked
        TERMINATED                    //uThread is done and should be terminated
    } state;

    /* hold joinable state of the uThread */
    enum class JState : std::uint8_t {
        DETACHED,          //Detached
        JOINABLE,          //Joinable
        JOINING            //Joining
    } jState;

    //TODO: Add a function to check uThread's stack for overflow ! Prevent overflow or throw an exception or error?
    //TODO: Add a debug object to project, or a dtrace or lttng functionality

    /**
     * The main and only constructor for uThread. uThreads are not supposed to
     * be created by using the constructor. The memory used to save the uThread
     * object is allocated at the beginning of its own stack. Thus, by freeing
     * the stack memory uThread object is being destroyed as well. Therefore,
     * the implicit destructor is not necessary.
     */
    uThread(vaddr sb, size_t ss) :
            stackPointer(vaddr(this)), stackBottom(sb), stackSize(ss), state(
                    State::INITIALIZED), uThreadID(uThreadMasterID++), currentCluster(
                    nullptr), jState(JState::DETACHED), homekThread(nullptr), utvar(nullptr) {
        totalNumberofUTs++;
    }

    /* Create a stack with the given size */
    static vaddr createStack(size_t);

    /* in order to avoid allocating memory over and over again, uThreads
     * are not completely destroyed after termination and are pushed to
     * a uThreadCache container. When the program asks for a new uThread
     * this cache is checked, if there is cached uThreads it will be used
     * instead of allocating new memory.
     */
    static uThreadCache utCache;        //data structure to cache uThreads

    /*
     * Statistics variables
     */
    //TODO: Add more variables, number of suspended, number of running ...
    static std::atomic_ulong totalNumberofUTs;  //Total number of existing uThreads
    static std::atomic_ulong uThreadMasterID;   //The main ID counter




    /*
     * Destroys the uThread by freeing the memory allocated on the stack.
     * Since uThread object is saved on its own stack it destroys the uThread
     * object as well.
     * If the uThreadCache is not full, this function do not destroy the
     * object and push it to the uThreadCache.
     * The object is destroyed either due to the cache being full, or the bool
     * parameter pass to it, which means force destroying the object, is true.
     */
    virtual void destory(bool);

    /*
     * This function is used to recycle the uThread as it is pushed in
     * uThreadCache. It causes the stack pointer points to the beginning of
     * the stack and change the status of the uThread to INITIALIZED.
     */
    void reset();

    /*
     * Used to suspend the uThread. Pass a function and an argument
     * to be called after the context switch.
     * It is normally the case that the uThread needs to hold on to a lock
     * or perform some maintenance after the context is switched.
     */
    void suspend(funcvoid2_t, void*);

    //Function to invoke the run function of a uThread
    static void invoke(funcvoid3_t, ptr_t, ptr_t, ptr_t) __noreturn;

public:
    /// uThread cannot be copied or assigned
    uThread(const uThread&) = delete;

    /// @copydetails uThread(const uThread&)
    const uThread& operator=(const uThread&) = delete;

    /**
     * @brief Create a uThread with a given stack size
     * @param ss stack size
     * @param joinable Whether this thread is joinable or detached
     * @return a pointer to a new uThread
     *
     * This function relies on a uThreadCache structure
     * and does not always allocate the stack.
     */
    static uThread* create(size_t ss, bool joinable=false);

    /**
     * @brief Create a uThread with default stack size
     * @param joinable Whether this thread is joinable or detached
     * @return a pointer to a new uThread
     */
    static uThread* create(bool joinable=false) {
        return create(defaultStackSize, joinable);
    }

    /**
     * @brief start the uThread by calling the function passed to it
     * @param cluster The cluster that function belongs to.
     * @param func a pointer to a function that should be executed by the uThread.
     * @param arg1 first argument of the function (can be nullptr)
     * @param arg2 second argument of the function (can be nullptr)
     * @param arg3 third argument of the function (can be nullptr)
     *
     * After creating the uThread and allocating the stack, the start() function
     * should be called to get the uThread going.
     */
    void start(const Cluster& cluster, ptr_t func, ptr_t arg1 = nullptr,
            ptr_t arg2 = nullptr, ptr_t arg3 = nullptr);

    /**
     * @brief Causes uThread to yield
     *
     * uThread give up the execution context and place itself back on the
     * ReadyQueue of the Cluster. If there is no other uThreads available to
     * switch to, the current uThread continues execution.
     */
    static void yield();

    /**
     * @brief Terminates the uThread
     *
     * By calling this function uThread is being terminated and uThread object
     * is either destroyed or put back into the cache.
     */
    static void terminate();

    /**
     * @brief Move the uThread to the provided cluster
     * @param cluster
     *
     * This function is used to migrate the uThread to another Cluster.
     * Migration is useful specially if clusters form a pipeline of execution.
     */
    static void migrate(Cluster*);

    /**
     * @brief Resumes the uThread. If uThread is blocked or is waiting on IO
     * it will be placed back on the ReadyQueue.
     */
    void resume();

    /**
     * @brief Wait for uThread to finish execution and exit
     * @return Whether join was successful or failed
     */
    bool join();

    /**
     * @brief Detach a joinable thread.
     */
    void detach();


    /**
     * @brief return the current Cluster uThread is executed on
     * @return the current Cluster uThread is executed on
     */
    Cluster& getCurrentCluster() const {
        return *currentCluster;
    }

    /**
     *
     * @return Total number of uThreads in the program
     *
     * This number does not include mainUT or IOUTs
     */
    static uint64_t getTotalNumberofUTs() {
        return totalNumberofUTs.load();
    }

    /**
     * @brief get the ID of this uThread
     * @return ID of the uThread
     */
    uint64_t getID() const {
        return uThreadID;
    }

    /**
     * @brief Get a pointer to the current running uThread
     * @return pointer to the current uThread
     */
    static uThread* currentUThread();
};

#endif /* UTHREADS_UTHREAD_H_ */
#include "Cluster.h"
