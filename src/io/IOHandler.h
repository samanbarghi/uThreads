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
#include <sys/socket.h>
#include <mutex>
#include "runtime/uThread.h"
#include "runtime/kThread.h"
#include "generic/ArrayQueue.h"

#define POLL_READY  ((uThread*)1)
#define POLL_WAIT   ((uThread*)2)

class Connection;
/**
 * @cond  HIDDEN_SYMBOLS
 * @class PollData
 * @brief Used to maintain file descriptor status while polling for activity
 *
 */
class PollData : public IntrusiveQueue<PollData>::Link{
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

    //Mutex that protects this  PollData
    std::mutex mtx;
    //file descriptor
    int fd = -1;

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
    uThread* rut = nullptr;
    uThread* wut = nullptr;

    /** Whether the fd is closing or not */
    bool closing;

    /** Whether the fd was added to epoll or not **/
    bool opened;

    /**
     * Reset the variables. Used when PollData is recycled to be used
     * for the same FD. The mutex should always be acquired before calling
     * this function.
     */
    void reset(){
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
    PollData( int fd) : fd(fd), closing(false), opened(false){};

    /**
     * @brief Create a PollData structure but do not assign any fd
     *
     * Usually used before accept
     */
    PollData(): fd(-1), closing(false), opened(false){};
    PollData(const PollData&) = delete;
    const PollData& operator=(const PollData&) = delete;
    ~PollData(){};

};

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
class PollCache{
    friend class IOHandler;
    friend class Connection;
protected:
    IntrusiveQueue<PollData> cache;
    std::mutex mtx;

    PollData* getPollData(){
        std::unique_lock<std::mutex> mlock(mtx);
        if(cache.empty()){
            for(int i=0 ; i < 128; i++){
               PollData* pd = new PollData();
               cache.push(*pd);
            }
        }
        PollData* pd = cache.front();
        cache.pop();
        return pd;
    }

    void pushPollData(PollData* pd){
        std::unique_lock<std::mutex> mlock(mtx);
        cache.push(*pd);
    }
};
/*
 * This class is a virtual class to provide nonblocking I/O
 * to uThreads. select/poll/epoll or other type of nonblocking
 * I/O can have their own class that inherits from IOHandler.
 * The purpose of this class is to be used as a thread local
 * I/O handler per kThread.
 */
class IOHandler{
    friend class Connection;
    friend class Cluster;
    friend class ReadyQueue;

protected:
    /* polling flags */
    enum Flag {
        UT_IOREAD    = 1 << 0,                           //READ
        UT_IOWRITE   = 1 << 1                            //WRITE
    };

    PollCache pollCache;

    ArrayQueue<uThread, EPOLL_MAX_EVENT> bulkArray;
    moodycamel::ProducerToken ptok;

    virtual int _Open(int fd, PollData &pd) = 0;         //Add current fd to the polling list, and add current uThread to IOQueue
    virtual int  _Close(int fd) = 0;
    virtual void _Poll(int timeout)=0;                ///


    void block(PollData &pd, bool isRead);
    void inline unblock(PollData &pd, int flag);
    void inline unblockBulk(PollData &pd, int flag);

    Cluster*    localCluster;       //Cluster that this Handler belongs to
    kThread    ioKT;               //IO kThread

    //Polling IO Function
   static void pollerFunc(void*) __noreturn;


   //Variables for bulk push to readyQueue
   IntrusiveList<uThread> bulkQueue;
   size_t bulkCounter;

    IOHandler(Cluster&);
    void PollReady(PollData &pd, int flag);                   //When there is notification update pollData and unblock the related ut
    void PollReadyBulk(PollData &pd, int flag, bool isLast);                   //When there is notification update pollData and unblock the related ut
   ~IOHandler(){};  //should be protected

public:
    /* public interfaces */
   void open(PollData &pd);
   int close(PollData &pd);
   void wait(PollData& pd, int flag);
   void poll(int timeout, int flag);
   void reset(PollData &pd);
   //dealing with uThreads


   //create an instance of IOHandler based on the platform
   static IOHandler* create(Cluster&);
};

/* epoll wrapper */
class EpollIOHandler : public IOHandler {
    friend IOHandler;
private:
    static const int MAXEVENTS  = EPOLL_MAX_EVENT;//Maximum number of events this thread can monitor, TODO: do we want this to be modified?
    int epoll_fd = -1;
    struct epoll_event* events;

    int _Open(int fd, PollData& pd);
    int  _Close(int fd);
    void _Poll(int timeout);

protected:
    EpollIOHandler(Cluster&);
    virtual ~EpollIOHandler();
};

/** @endcond */
#endif /* IOHANDLER_H_ */
