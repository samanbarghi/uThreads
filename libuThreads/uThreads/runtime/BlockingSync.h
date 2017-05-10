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

#ifndef UTHREADS_BLOKING_SYNC_H
#define UTHREADS_BLOKING_SYNC_H

#include "../generic/IntrusiveContainers.h"
#include <cassert>
#include <mutex>
#include <iostream>

namespace uThreads {
namespace runtime {
//TODO: implement timeouts for all synchronization and mutual exclusion variables
class Mutex;

class uThread;

/**
 * @class BlockingQueue
 * @brief A queue used to keep track of blocked uThreads
 *
 * This queue is a FIFO queue used to hold blocked uThreads on Mutex, Semaphore,
 * or Condition Variable.
 */
class BlockingQueue {
    friend class uThread;

    friend class Mutex;

    friend class OwnerLock;

    friend class ConditionVariable;

    friend class Semaphore;

    uThreads::generic::IntrusiveList<uThread> queue;

    // temporary fix to avoid including uThread.h
    uThread *getCurrentUThread();

 public:
    BlockingQueue() = default;

    ~BlockingQueue() {
        assert(empty());
    }

    /// @brief BlockingQueue cannot be copied or assigned
    BlockingQueue(const BlockingQueue &) = delete;

    /// @copybrief BlockingQueue(const BlockingQueue&)
    const BlockingQueue &operator=(const BlockingQueue &) = delete;

    ///@brief Whether the queue is empty or not
    bool empty() const {
        return queue.empty();
    }

    /**
     * @brief Suspends the uThread and add it to the queue
     * @param lock a mutex to be released after blocking
     * @return whether the suspension was successful or not
     *
     *  Suspends the uThread and adds it to the BlockingQueue.
     *
     */
    bool suspend(std::mutex &lock);

    /// @copydoc suspend(std::mutex& lock);
    bool suspend(Mutex &);
//  bool suspend(mutex& lock, mword timeout);

    /**
     * @brief Unblock one blocked uThread, used for OwnerLock
     * @param lock mutex  to be released after signal is done
     * @param owner passed to support atomic setting of Mutex::owner
     * @return true if a uThread was unblocked, and false otherwise
     */
    bool signal(std::mutex &lock, uThread *&owner); //Used along with Mutex

    /**
     * @brief unblock one blocked, used  for Mutex
     * @param lock mutex to be released after signal is done
     * @return true if a uThread was unblocked, and false otherwise
     */
    bool signal(std::mutex &lock) {
        uThread *dummy = nullptr;
        return signal(lock, dummy);
    }

    /**
     * @brief unblock one blocked uThread, used for ConditionVariable
     * @param Mutex that is released after signal is done
     * @return true if a uThread was unblocked, and false otherwise
     */
    bool signal(Mutex &);

    /**
     * @brief unblock all blocked uThreads, used for Condition Variable
     * @param Mutex to be released after signallAll is done
     */
    void signalAll(Mutex &);

    template<typename T>
    static void postSwitchFunc(void *ut, void *args) {
        assert(args != nullptr);
        assert(ut != nullptr);
        uThread *utt = (uThread *) ut;

        auto bqp = (std::pair<T *, BlockingQueue *> *) args;
        bqp->second->queue.push_back(*utt);
        bqp->first->unlock();
    }
};

/**
 * @class Mutex
 * @brief A user-level Mutex
 */
class Mutex {
    friend class ConditionVariable;

    friend class Semaphore;

    friend class BlockingQueue;

 protected:
    /*
     * std::mutex acting as the base lock. Usually this is implemented as a
     * spinlock, but since we are interested in blocking in user level and this
     * lock is only held for a small amount of time the possibilities that it
     * blocks the kThread for a long time is minute, and hence it is safe
     * to use a mutex as the base lock for now. This might change in the future.
     */
    std::mutex lock;
    /* The blocking queue used for this Mutex to block and unblock uThreads */
    BlockingQueue bq;
    /* Owner of the Mutex */
    uThread *owner;

    template<bool OL>
    // OL: owner lock
    bool internalAcquire(mword timeout) {

        lock.lock();
        //TODO: since assert is disabled during production, find another way
        //to make sure the following is happening
        //Mutex cannot have recursive locking
        assert(OL || owner != bq.getCurrentUThread());
        if (fastpath(owner != nullptr && (!OL || owner != bq.getCurrentUThread()))) {
            return bq.suspend(lock); // release lock, false: timeout
        } else {
            /*
             * End up here if:
             * 	- No one is holding the lock
             * 	- Someone is holding the lock and:
             * 		+ The type is OwnlerLock and the calling thread is holding the lock
             * Otherwise, block and wait for the lock
             */
            owner = bq.getCurrentUThread();
            lock.unlock();
            return true;
        }
    }

    void internalRelease() {
        // try baton-passing, if yes: 'signal' releases lock
        if (fastpath(bq.signal(lock, owner))) return;
        owner = nullptr;
        lock.unlock();
    }

 public:
    Mutex() : owner(nullptr) {}

    /**
     * @brief acquire the mutex
     * @return true if it was acquired, false otherwise
     *
     * The return value is only for when timeouts are implemented
     */
    template<bool OL = false>
    bool acquire() { return internalAcquire<OL>(0); }

    //TODO: implement tryAcquire after timeouts are implemented
    //  template<bool OL=false>
    //  bool tryAcquire(mword t = 0) { return internalAcquire<true,OL>(t); }

    /**
     * @brief release the Mutex
     */
    void release() {
        lock.lock();
        // Attempt to release lock by non-owner
        assert(owner == bq.getCurrentUThread());
        internalRelease();
    }

    // temporary solution for the postSwitchFunc
    void unlock() { release(); }
};

/**
 * @class OwnerLock
 * @brief an Owner Mutex where owner can recursively acquire the Mutex
 */
class OwnerLock : protected Mutex {
    /*
     * Counter to keep track of how many time this has been acquired
     * by the owner.
     */
    mword counter;
 public:
    /**
     * @brief Create a new OwnerLock
     */
    OwnerLock() :
            counter(0) {
    }

    /**
     * @brief Acquire the OwnerLock
     * @return The number of times current owner acquired the lock
     */
    mword acquire() {
        if (internalAcquire<true>(0))
            return ++counter;
        else
            return 0;
    }
    //  mword tryAcquire(mword t = 0) {
    //    if (internalAcquire<true,true>(t)) return ++counter;
    //    else return 0;
    //  }
    /**
     * @brief Release the OwnerLock once
     * @return The number of times current owner acquired the lock, if lock is
     * released completely the result is 0
     */
    mword release() {
        lock.lock();
        assert(owner == bq.getCurrentUThread());
        if (--counter == 0)
            internalRelease();
        else
            lock.unlock();
        return counter;
    }
};

/**
 * @class ConditionVariable
 * @brief A user level condition variable
 *
 * User-level Condition Variable blocks only in user-level by suspending
 * the uThreads instead of blocking the kernel threads.
 */
class ConditionVariable {
    BlockingQueue bq;

 public:
    /**
     * @brief Block uThread on the condition variable using the provided mutex
     * @param mutex used to synchronize access to the condition
     */
    void wait(Mutex &mutex) {
        bq.suspend(mutex);
        mutex.acquire();
    }

    /**
     * @brief Unblock one uThread waiting on the condition variable
     * @param mutex The mutex to be released after unblocking is done
     */
    void signal(Mutex &mutex) {
        if (slowpath(!bq.signal(mutex))) mutex.release();
    }

    /**
     * @brief unblock all uThreads waiting on the condition variable
     * @param mutex The mutex to be released after unblocking is done
     */
    void signalAll(Mutex &mutex) { bq.signalAll(mutex); }

    /**
     * @brief Whether the waiting list is empty or not
     * @return Whether the waiting list is empty or not
     */
    bool empty() { return bq.empty(); }
};

/**
 * @class Semaphore
 * @brief A user-level Semaphore
 *
 * blocks in user levle by blocking uThreads and does not cause kernel threads
 * to block.
 */
class Semaphore {
    Mutex mutex;
    BlockingQueue bq;
    mword counter;

    template<bool TO>
    bool internalP(mword timeout) {
        if (fastpath(counter < 1)) {
            return bq.suspend(mutex);  // release lock, false: timeout
        } else {
            counter -= 1;
            mutex.release();
            return true;
        }
    }

 public:
    /**
     * @brief Create a new Semaphore
     * @param c Initial value of the Semaphore
     */
    explicit Semaphore(mword c = 0) : counter(c) {}

    /**
     * @brief Decrement the value of the Semaphore
     * @return Whether it was successful or not
     */
    bool P() {
        mutex.acquire();
        return internalP<false>(0);
    }

    //	bool tryP(mword t = 0, SpinLock* l = nullptr) {
    //		if (l) lock.acquire(*l); else lock.acquire();
    //		return internalP<true>(t);
    //	}

    /**
     * @brief increment the value of the Semaphore
     */
    void V() {
        mutex.acquire();
        // try baton-passing, if yes: 'signal' releases lock
        if (fastpath(bq.signal(mutex))) return;
        counter += 1;
        mutex.release();
    }
};
}  // namespace runtime
}  // namepace uthreads
#endif /* UTHREADS_BLOKING_SYNC_H */
