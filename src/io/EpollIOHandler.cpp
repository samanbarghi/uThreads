/*
 * EpollIOHandler.cpp
 *
 *  Created on: Aug 13, 2015
 *      Author: Saman Barghi
 */

#include "IOHandler.h"
#include "kThread.h"
#include <iostream>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>

EpollIOHandler::EpollIOHandler() : IOHandler() {
    epoll_fd = epoll_create1 (EPOLL_CLOEXEC);               //Assuming kernel version >= 2.9
    														//TODO: Make sure this is backward compatible with kernel < 2.9
    if (epoll_fd == -1){
        std::cerr << "epoll_create: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }
//    std::cout << "EPOLL_FD" << epoll_fd << std::endl;

    events = (epoll_event*)calloc(MAXEVENTS, sizeof(struct epoll_event));
}

void EpollIOHandler::_Open(int fd, PollData* pd){
//    std::cout << "FD (" << fd << "): EPOLL OPEN" << std::endl;
    struct epoll_event ev;
    ev.events = EPOLLIN|EPOLLOUT|EPOLLRDHUP|EPOLLET;
    ev.data.ptr = (void*)pd;


    while( !epoll_ctl(epoll_fd,EPOLL_CTL_ADD, fd, &ev) && errno == EAGAIN)
    {
        //TODO:handle other epoll errors
        //std::cerr << "epoll_ctl : " << errno  << std::endl;
    }

}

void EpollIOHandler::_Poll(int timeout){
    if(!epoll_fd)
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

    for(int i = 0; i < n; i++) {
        ev = &events[i];
        //std::cout << "FD(" << ev->data.fd << ") EPOLL RETURN RESULT" << std::endl;
        if(ev->events == 0)
            continue;
        mode = 0;
        if(ev->events & (EPOLLIN|EPOLLRDHUP|EPOLLHUP|EPOLLERR))
            mode |= UT_IOREAD;
        if(ev->events & (EPOLLOUT|EPOLLHUP|EPOLLERR))
            mode |= UT_IOWRITE;
        if(mode)
            this->PollReady((PollData*) ev->data.ptr , mode);
    }

}


EpollIOHandler::~EpollIOHandler() {
    close(epoll_fd);
    free(events);
}

