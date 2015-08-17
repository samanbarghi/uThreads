/*
 * EpollIOHandler.cpp
 *
 *  Created on: Aug 13, 2015
 *      Author: Saman Barghi
 */

#include "IOHandler.h"
#include "kThread.h"
#include "ListAndUnlock.h"
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

    events = (epoll_event*)calloc(MAXEVENTS, sizeof(struct epoll_event));
}

void EpollIOHandler::_ModifyFD(int fd, int flag){
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = 0;

    if(flag & UT_IOREAD) ev.events |= EPOLLIN;
    if(flag & UT_IOWRITE) ev.events |= EPOLLOUT;

    //TODO:check whether the fd already exists in the list of monitored descriptors
    bool fd_exist = ( IOTable.find(fd) != IOTable.end() );

    if( fastpath(!epoll_ctl(epoll_fd, fd_exist ? EPOLL_CTL_MOD : EPOLL_CTL_ADD, fd, &ev)))
    {
        std::cerr << "epoll_ctl : " << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    //Now that we added the fd for listening move the current thread to the local map
    //TODO: There is a problem here, what if epoll singals before this is added to the list? with level triggered
    //There is no problem but for edge-triggered this should be handled
    //TODO: is it possible to lose the uThread if we just add it to events.data.ptr, and retrieve it later?
    //This requires the uThread be moved to a blocking queue in kThread or the Cluster and moved back to ready queue when
    //fd is ready.

    MapAndUnlock<int>* mau = new MapAndUnlock<int>(&this->IOTable, fd, nullptr);
    kThread::currentKT->currentUT->suspend(mau);

}

void EpollIOHandler::_Poll(int timeout){

}


EpollIOHandler::~EpollIOHandler() {
    close(epoll_fd);
    free(events);
}

