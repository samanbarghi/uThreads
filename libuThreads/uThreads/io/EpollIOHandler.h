/*
 * EpollIOHandler.h
 *
 *  Created on: May 20, 2016
 *      Author: saman
 */

#ifndef SRC_IO_EPOLLIOHANDLER_H_
#define SRC_IO_EPOLLIOHANDLER_H_

#include <sys/epoll.h>

namespace uThreads {
namespace io {
using ::epoll_event;

/* epoll wrapper */
class IOPoller {
    friend IOHandler;
 private:
    // Maximum number of events this thread can monitor,
    // TODO(saman): do we want this to be modified?
    static const int MAXEVENTS = 256;
    int epoll_fd = -1;
    struct epoll_event *events;
    IOHandler &ioh;

    int _Open(int fd, PollData &pd);

    int _Close(int fd);

    ssize_t _Poll(int timeout);

 protected:
    IOPoller(IOHandler &);

    virtual ~IOPoller();
};
}  // namespace io
}  // namespace uThreads

#endif /* SRC_IO_EPOLLIOHANDLER_H_ */
