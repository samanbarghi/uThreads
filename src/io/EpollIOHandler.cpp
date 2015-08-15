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
    if (epoll_fd == -1){
        std::cerr << "epoll_create: " << errno << std::endl;
        exit(EXIT_FAILURE);
    }
    events = (epoll_event*)calloc(MAXEVENTS, sizeof(struct epoll_event));
}

void EpollIOHandler::do_addfd(int fd, int flag){
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = 0;

    if(flag & UTIOREAD) ev.events |= EPOLLIN;
    if(flag & UTIOWRITE) ev.events |= EPOLLOUT;

    if( epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        std::cerr << "epoll_ctl : EPOLL_CTL_ADD : " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    IOQueue.insert(std::make_pair<int, uThread*> (fd, kThread::currentKT->currentUT));

}

void EpollIOHandler::do_poll(int timeout){

}


EpollIOHandler::~EpollIOHandler() {
    close(epoll_fd);
    free(events);
}

