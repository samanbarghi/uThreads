/*
 * IOHandler.h
 *
 *  Created on: Aug 13, 2015
 *      Author: Saman Barghi
 */

#ifndef IOHANDLER_H_
#define IOHANDLER_H_

#include <unordered_map>
#include "uThread.h"
/*
 * This class is a virtual class to provide nonblocking I/O
 * to uThreads. select/poll/epoll or other type of nonblocking
 * I/O can have their own class that inherits from IOHandler.
 * The purpose of this class is to be used as a thread local
 * I/O handler per kThread.
 */
class IOHandler{
    virtual void do_addfd(int fd, int flag) = 0;         //Add current fd to the polling list, and add current uThread to IOQueue
    /*
     * Timeout can be int for epoll, timeval for select or timespec for poll.
     * Here the assumption is timeout is passed in milliseconds as an integer, similar to epoll.
     */
    virtual void do_poll(int timeout)=0;                ///
protected:
    IOHandler(){};

    /* polling flags */
    enum flags {
        UTIOREAD    = 1 << 0,                           //READ
        UTIOWRITE   = 1 << 1                            //WRITE
    };
    /*
     * When a uThread request I/O, the device will be polled
     * and uThread should be added to IOTable.
     * This data structure maps file descriptors to uThreads, so when
     * the device is ready, this structure is used as a look up table to
     * put the waiting uThreads back on the related readyQueue.
     */
    std::unordered_map<int, uThread**> IOTable;
   ~IOHandler(){};
public:
    /* public interfaces */
   void addFileDescriptor(int fd, int flag){ do_addfd(fd,flag);};
   void poll(int timeout, int flag){do_poll(timeout);};

   //dealing with uThreads
};

/* epoll wrapper */
class EpollIOHandler : public IOHandler {
private:
    static const int MAXEVENTS  = 1024;                           //Maximum number of events this thread can monitor, TODO: can set it at compile time
    int epoll_fd;
    struct epoll_event* events;

    void do_addfd(int fd, int flag);
    void do_poll(int timeout);
public:
    EpollIOHandler();
    virtual ~EpollIOHandler();
};


#endif /* IOHANDLER_H_ */
