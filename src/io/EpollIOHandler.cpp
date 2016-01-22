/*
 * EpollIOHandler.cpp
 *
 *  Created on: Aug 13, 2015
 *      Author: Saman Barghi
 */

#include "IOHandler.h"
#include "runtime/kThread.h"
#include <iostream>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>

EpollIOHandler::EpollIOHandler(Cluster& cluster) : IOHandler(cluster) {
    epoll_fd = epoll_create1 (EPOLL_CLOEXEC);               //Assuming kernel version >= 2.9
    														//TODO: Make sure this is backward compatible with kernel < 2.9
    if (epoll_fd == -1){
        std::cerr << "epoll_create: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }
//    std::cout << "EPOLL_FD" << epoll_fd << std::endl;

    events = (epoll_event*)calloc(MAXEVENTS, sizeof(struct epoll_event));
}

int EpollIOHandler::_Open(int fd, PollData *pd){
//    std::cout << "FD (" << fd << "): EPOLL OPEN" << std::endl;
    struct epoll_event ev;
    ev.events = EPOLLIN|EPOLLOUT|EPOLLRDHUP|EPOLLET;
    ev.data.ptr = (void*)pd;

    int res = epoll_ctl(epoll_fd,EPOLL_CTL_ADD, fd, &ev);
    if(slowpath(res < 0))
        std::cerr<< "EPOLL ADD ERROR: " << errno << std::endl;
    return res;
}

int EpollIOHandler::_Close(int fd){
   struct epoll_event ev;
   int res;

  res = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &ev);
  if(slowpath(res < 0 && errno != 2))
      std::cerr<< "EPOLL DEL ERROR: " << errno << std::endl;
  return res;
}

/*
 * Since epoll events are in edge-triggered mode for performance
 * reasons the event contains both EPOLLIN and EPOLLOUT. This can
 * cause unnecessary events being triggered for write or read when
 * not needed. e.g., when read reaches EAGAIN but there is no interest
 * for write readiness, epoll returns both EPOLLIN and EPOLLOUT for the
 * specified file descriptor. Thus, in processing the returned events in
 * IOHandeler, it is important to make sure this does not add extra overhead.
 */

void EpollIOHandler::_Poll(int timeout){
    if(slowpath(!epoll_fd))
        return;

    int32_t n, mode;
    struct epoll_event *ev;

    //TODO: dedicated thread always blocks but others should not
    n = epoll_wait(epoll_fd, events, MAXEVENTS, timeout);
    while( n < 0){
       //TODO: Throw an exception
        std::cout << "EPOLL ERROR" << std::endl;
        n = epoll_wait(epoll_fd, events, MAXEVENTS, timeout);
    }

    //timeout
    if(slowpath(n == 0 ))
        return;

    for(int i = 0; i < n; i++) {
        ev = &events[i];
        //std::cout << "FD(" << ev->data.fd << ") EPOLL RETURN RESULT" << std::endl;
        if(slowpath(ev->events == 0))
            continue;
        mode = 0;
        if(ev->events & (EPOLLIN|EPOLLRDHUP|EPOLLHUP|EPOLLERR))
            mode |= UT_IOREAD;
        if(ev->events & (EPOLLOUT|EPOLLHUP|EPOLLERR))
            mode |= UT_IOWRITE;
        if(fastpath(mode))
            this->PollReadyBulk( *(PollData*) ev->data.ptr , mode, i==n-1);
    }

}

EpollIOHandler::~EpollIOHandler() {
    ::close(epoll_fd);
    free(events);
}

