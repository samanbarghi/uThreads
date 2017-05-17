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

#ifndef UTHREADS_IOHANDLER_H_
#define UTHREADS_IOHANDLER_H_

#include <unordered_map>
#include <vector>
#include <mutex>
#include "../runtime/uThread.h"
#include "../runtime/kThread.h"
#include "../generic/Semaphore.h"
#include <sys/socket.h>

#define POLL_READY  ((uThread*)1)
#define POLL_WAIT   ((uThread*)2)

namespace uThreads {
namespace io {
using uThreads::runtime::kThread;
using uThreads::runtime::uThread;

class Connection;

/**
 * @cond  HIDDEN_SYMBOLS
 * @class PollData
 * @brief Used to maintain file descriptor status while polling for activity
 *
 */
class PollData : public uThreads::generic::Link<PollData> {
    friend IOHandler;
    friend Connection;
 private:

    /*
     * Potentially there is only the poller thread and maximum two
     * other uThreads (one for read, one for write) can contend for
     * this mutex, the overhead of acquiring and releasing this
     * mutex under linux is not that high (due to futex operating in
     * user-level under no contention). However, it might be necessary
     * to change it to a compare-and-swap or reader/writer lock if this
     * ever needed to be portable.
     * Another reason for lack of contention is that epoll is used in
     * edge-triggered and it is usually the case that either the poller
     * or the requesting uThread is updating the semaphore.
     */

    // First 64 bytes (CACHELINE_SIZE)
    /**
     * These semaphores are used to keep track of the state at which
     * the file descriptor is in. They can be in four modes:
     *
     * 1- nullptr: there is no activity nor any interest in the fd
     * 2- POLL_READY:  the underlying poller indicated that the fd is ready for
     *                  read or write.
     * 3- POLL_WAIT: uThread is trying to read/write and is not parked yet
     * 4- uThread*: uThread is parked and is waiting on fd to become ready
     *              for read/write
     */
    uThread *rut = nullptr;
    uThread *wut = nullptr;

    /** Whether the fd is closing or not */
    bool closing;

    /** Whether the fd was added to epoll or not **/
    bool opened;

    /** Whether this pd is about to block on read or write */
    bool isBlockingOnRead;

    //file descriptor
    int fd = -1;

    // First 64 bytes (CACHELINE_SIZE)

    //Mutex that protects this  PollData
    //std::mutex mtx;

    /**
     * Reset the variables. Used when PollData is recycled to be used
     * for the same FD. The mutex should always be acquired before calling
     * this function.
     */
    void reset() {
        fd = -1;
        rut = nullptr;
        wut = nullptr;
        closing = false;
        opened = false;
    };

 public:
    /**
     * @brief Create a PollData structure with the assigned fd
     * @param fd file descriptor
     */
    PollData(int fd) : fd(fd), closing(false), opened(false), isBlockingOnRead(false) {};

    /**
     * @brief Create a PollData structure but do not assign any fd
     *
     * Usually used before accept
     */
    PollData() : fd(-1), closing(false), opened(false), isBlockingOnRead(false) {};

    PollData(const PollData &) = delete;

    const PollData &operator=(const PollData &) = delete;

    ~PollData() {};
} __packed;

/*
 * PollCache is used to cache PollData objects and avoid
 * nullptr exceptions after the file descriptor is closed.
 * e.g., after a connection is closed, epoll might generate
 * a notification returning the pointer to PollData back to
 * the IOHandler. At this point if the PollData object is
 * destroyed, the pointer is not valid anymore and segfault
 * can happen. By allocating space in PollCache and returning
 * the pointer to it after the connection is closed, the pointer
 * is always valid. TODO: it might be necessary to deal with stale
 * notifications in the future, but for now the assumption is that
 * a stale notification can only cause an unnecessary unblock of the
 * new uThread(if the PollData is assigned to a new Connection).
 */
class PollCache {
    friend class IOHandler;

    friend class Connection;

 protected:
    uThreads::generic::IntrusiveQueue<PollData> cache;
    std::mutex mtx;

    PollData *getPollData() {
        std::unique_lock<std::mutex> mlock(mtx);
        if (cache.empty()) {
            for (int i = 0; i < 128; i++) {
                PollData *pd = new PollData();
                cache.push(*pd);
            }
        }
        PollData *pd = cache.front();
        cache.pop();
        return pd;
    }

    void pushPollData(PollData *pd) {
        std::unique_lock<std::mutex> mlock(mtx);
        cache.push(*pd);
    }
};
}  // namespace io
}  // namespace uThreads
#if defined (__linux__)

#include "EpollIOHandler.h"

#else
#error unsupported system: only __linux__ supported at this moment
#endif

namespace uThreads {
namespace io {
/*
 * This class is a virtual class to provide nonblocking I/O
 * to uThreads. select/poll/epoll or other type of nonblocking
 * I/O can have their own class that inherits from IOHandler.
 * The purpose of this class is to be used as a thread local
 * I/O handler per kThread.
 */
class IOHandler {
    friend class Connection;

    friend class uThreads::runtime::Cluster;

    friend class ReadyQueue;

    friend class IOPoller;

    friend class uThreads::runtime::Scheduler;

 protected:

    static IOHandler iohandler;

    // Variables for bulk push to readyQueue
    size_t unblockCounter;

    std::atomic_flag isPolling;

    uThreads::generic::semaphore sem;

    kThread ioKT;               // IO kThread

    PollCache pollCache;

    IOPoller poller;

    /* polling flags */
    enum Flag {
        UT_IOREAD = 1 << 0,     // READ
        UT_IOWRITE = 1 << 1     // WRITE
    };

    void block(PollData &pd, bool isRead);

    bool inline unblock(PollData &pd, bool isRead);

    static void postSwitchFunc(void *ut, void *args);

    //Polling IO Function
    static void pollerFunc(void *) __noreturn;

    IOHandler();

    // When there is notification update pollData and unblock the related ut
    void PollReady(PollData &pd,
                   int flag);
    ~IOHandler() {}

 public:
    /* public interfaces */
    void open(PollData &pd);

    int close(PollData &pd);

    void wait(PollData &pd, int flag);

    ssize_t poll(int timeout, int flag);

    ssize_t nonblockingPoll();

    void reset(PollData &pd);
    //dealing with uThreads

    // assign an IOHandler to a kThread based on the provided policy
    static IOHandler* getkThreadIOHandler(kThread& kt);

    // assign an IOHandler to a Cluster based on the provided policy
    static IOHandler* getClusterIOHandler(uThreads::runtime::Cluster& cluster);
};
/** @endcond */
}  // namespace io
}  // namespace uThreads
#endif /* IOHANDLER_H_ */
