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

#include "IOHandler.h"
#include "runtime/kThread.h"
#include <iostream>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>

EpollIOHandler::EpollIOHandler(Cluster& cluster) : IOHandler(cluster) {
    //Assuming kernel version >= 2.9
    epoll_fd = epoll_create1 (EPOLL_CLOEXEC);
   	//TODO: Make sure this is backward compatible with kernel < 2.9
    if (epoll_fd == -1){
        std::cerr << "epoll_create: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }
    events = (epoll_event*)calloc(MAXEVENTS, sizeof(struct epoll_event));
}

int EpollIOHandler::_Open(int fd, PollData &pd){
    struct epoll_event ev;
    ev.events = EPOLLIN|EPOLLOUT|EPOLLRDHUP|EPOLLET;
    ev.data.ptr = (void*)&pd;

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
        std::cout << "EPOLL ERROR:" << errno << std::endl;
        n = epoll_wait(epoll_fd, events, MAXEVENTS, timeout);
    }

    //timeout
    if(slowpath(n == 0 ))
        return;

    for(int i = 0; i < n; i++) {
        ev = &events[i];
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

