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

IOPoller::IOPoller(IOHandler& i) : ioh(i) {
    //Assuming kernel version >= 2.9
    epoll_fd = epoll_create1 (EPOLL_CLOEXEC);
   	//TODO: Make sure this is backward compatible with kernel < 2.9
    if (epoll_fd == -1){
        std::cerr << "epoll_create: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }
    events = (epoll_event*)calloc(MAXEVENTS, sizeof(struct epoll_event));
}

int IOPoller::_Open(int fd, PollData &pd){
    struct epoll_event ev;
    ev.events = EPOLLIN|EPOLLOUT|EPOLLRDHUP|EPOLLET;
    ev.data.ptr = (void*)&pd;

    int res = epoll_ctl(epoll_fd,EPOLL_CTL_ADD, fd, &ev);
    if(slowpath(res < 0))
        std::cerr<< "EPOLL ADD ERROR: " << errno << std::endl;
    return res;
}

int IOPoller::_OpenLT(int fd, PollData &pd){
    struct epoll_event ev;
    ev.events = EPOLLIN|EPOLLRDHUP;
    ev.data.ptr = (void*)&pd;

    int res = epoll_ctl(epoll_fd,EPOLL_CTL_ADD, fd, &ev);
    if(slowpath(res < 0))
        std::cerr<< "EPOLL ADD ERROR: " << errno << std::endl;
    return res;
}

int IOPoller::_Close(int fd){
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

ssize_t IOPoller::_Poll(int timeout){
    assert(epoll_fd);

    int32_t n, mode;
    struct epoll_event *ev;

    //TODO: dedicated thread always blocks but others should not
    n = epoll_wait(epoll_fd, events, MAXEVENTS, timeout);

    if (n == -1) {
        if (errno != EINTR) {
            return (-1);
        }
        return (0);
    }

    //timeout
    if(n == 0 ) return 0;

    for(int i = 0; i < n; i++) {
        ev = &events[i];
        if(slowpath(ev->events == 0))
            continue;
        mode = 0;
        if(ev->events & (EPOLLIN|EPOLLRDHUP|EPOLLHUP|EPOLLERR))
            mode |= IOHandler::UT_IOREAD;
        if(ev->events & (EPOLLOUT|EPOLLHUP|EPOLLERR))
            mode |= IOHandler::UT_IOWRITE;
        if(fastpath(mode))
            ioh.PollReady( *(PollData*) ev->data.ptr , mode);
    }
    return n;

}

IOPoller::~IOPoller() {
    ::close(epoll_fd);
    free(events);
}

