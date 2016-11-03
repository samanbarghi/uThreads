/*
 * EpollIOHandler.h
 *
 *  Created on: May 20, 2016
 *      Author: saman
 */

#ifndef SRC_IO_EPOLLIOHANDLER_H_
#define SRC_IO_EPOLLIOHANDLER_H_


/* epoll wrapper */
class IOPoller {
    friend IOHandler;
private:
    static const int MAXEVENTS  = 32;//Maximum number of events this thread can monitor, TODO: do we want this to be modified?
    int epoll_fd = -1;
    struct epoll_event* events;
    IOHandler& ioh;

    int _Open(int fd, PollData& pd);
    int  _Close(int fd);
    ssize_t _Poll(int timeout);

protected:
    IOPoller(IOHandler&);
    virtual ~IOPoller();
};



#endif /* SRC_IO_EPOLLIOHANDLER_H_ */
