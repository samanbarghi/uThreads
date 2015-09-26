/*
 * IOHandler.cpp
 *
 *  Created on: Aug 17, 2015
 *      Author: Saman Barghi
 */
#include "IOHandler.h"
#include "kThread.h"
#include "uThread.h"
#include <unistd.h>
#include <sys/types.h>

void IOHandler::open(int fd, int flag){
//    std::cout << "FD (" << fd << "): IOHandler::Open" << std::endl;

    PollData* pd;
    std::mutex* sndMutex;
    bool isIoMutex = false;

    std::unique_lock<std::mutex> mlock(iomutex);
    auto cfd =IOTable.find(fd) ;

    /*
     * If fd is not being monitored yet, add it to the underlying io interface
     */
    if( cfd == IOTable.end()){
        pd = new PollData(fd);
        sndMutex = &iomutex;
        isIoMutex = true;
    }else{ //otherwise load the pd
        pd = cfd->second;
        sndMutex = &(pd->mtx);
        isIoMutex = false;

    }

    std::unique_lock<std::mutex> pdlock(pd->mtx);
    if(!isIoMutex)
        mlock.unlock();

    //if new or newFD flag is set, add it to underlying poll structure
    if( pd->newFD ){
        pd->newFD = false;
        _Open(fd, pd);
    }
    //TODO:check other states
    if(flag & UT_IOREAD){
        if(slowpath(pd->readState == pd->READY))    //This is unlikely since we just did a nonblocking read
        {
//            std::cout << "FD(" << fd << "): Result is ready" << std::endl;
            pd->readState = pd->IDLE;               //consume the notification and return;
            return;
        }
        pd->rut = kThread::currentKT->currentUT;
        pd->readState = pd->PARKED;
    }
    if(flag & UT_IOWRITE)
    {
        if(slowpath(pd->writeSate == pd->READY)){
            pd->writeSate = pd->IDLE;              //consume the notification and return
            return;
        }
        pd->writeSate = pd->PARKED;
        pd->wut = kThread::currentKT->currentUT;
    }
    if(isIoMutex){
        mlock.release();
        pdlock.unlock();
    }else
        pdlock.release();

    MapAndUnlock<int, PollData>* mau = new MapAndUnlock<int,PollData>(&this->IOTable, fd, pd, sndMutex);
    kThread::currentKT->currentUT->suspend(mau);
//    std::cout << "Wake up from suspension" << std::endl;
    //when epoll returns this ut will be back on readyQueue and pick up from here
}
void IOHandler::poll(int timeout, int flag){

    _Poll(timeout);
}
void IOHandler::PollReady(PollData* pd, int flag){
    assert(pd != nullptr);

    uThread *rut = nullptr, *wut = nullptr;

    std::lock_guard<std::mutex> pdlock(pd->mtx);
    if(flag & UT_IOREAD){

        if(pd->readState == pd->IDLE){
            pd->readState = pd->READY;
            return;
        }else if(pd->readState == pd->PARKED){
            assert(pd->rut != nullptr);
            pd->readState = pd->IDLE;
            rut = pd->rut;
            pd->rut = nullptr;
//            std::cout << "Resume UT on fd " << fd << std::endl;
            rut->resume();
        }
        //if ready keep it that way? or should we do sth about it?
    }
    if(flag & UT_IOWRITE){
        if(pd->writeSate== pd->IDLE)
            pd->writeSate= pd->READY;
        else if(pd->writeSate == pd->PARKED){
            assert(pd->wut != nullptr);
            pd->writeSate = pd->IDLE;
            wut = pd->wut;
            pd->wut = nullptr;
            wut->resume();
        }
        //if ready keep it that way? or should we do sth about it?
    }
}

void IOHandler::defaultIOFunc(void*){
    //wait for IO
    //TODO: fix this
    usleep(1000000);
    while(true){
//       std::cout << "Waiting ... " << std::endl;
       kThread::ioHandler->poll(-1, 0);
   }
}

int IOHandler::accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
//    std::cout << "FD(" << sockfd << "): Calling accept" << std::endl;
    //check connection queue for wating connetions
    //Set the fd as nonblocking
    int res = ::accept4(sockfd, addr, addrlen, 0 );
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        //User level blocking using nonblocking io
        open(sockfd, UT_IOREAD);
        res = ::accept4(sockfd, addr, addrlen, 0);
    }
    //otherwise return the result
//    std::cout << "FD(" << res << "): Accept result"  << std::endl;
    /*
     * Updated the user-level cache
     */
    if(res>0)
    {
        std::lock_guard<std::mutex> mlock(iomutex);
        auto cfd = IOTable.find(res) ;
        if(cfd != IOTable.end()){
            std::lock_guard<std::mutex> pdlock(cfd->second->mtx);
            cfd->second->newFD = true;
        }
    }
    return res;

}
ssize_t IOHandler::recv(int sockfd, void *buf, size_t len, int flags){
    assert(buf != nullptr);

    ssize_t res = ::recv(sockfd, (void*)buf, len, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
//            std::cout << "EAGAIN on recv" << std::endl;
           //User level blocking using nonblocking io
           open(sockfd, UT_IOREAD);
           res = ::recv(sockfd, buf, len, flags);
    }
//    std::cout << "FD(" << sockfd << "): recv result: " << res  << std::endl;

    return res;
}
ssize_t IOHandler::send(int sockfd, const void *buf, size_t len, int flags){
    assert(buf != nullptr);

    ssize_t res = ::send(sockfd, buf, len, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        open(sockfd, UT_IOWRITE);
        res = ::send(sockfd, buf, len, flags);
    }
    return res;
}
