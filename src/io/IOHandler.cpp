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

    assert(fd < POLL_CACHE_SIZE);
    pd = &pollCache[fd];

    std::unique_lock<std::mutex> pdlock(pd->mtx);

    //if new or newFD flag is set, add it to underlying poll structure
    if( pd->newFD ){
        pd->newFD = false;
        _Open(fd, pd);
    }
    //TODO:check other states
    if(flag & UT_IOREAD){
        if(slowpath(pd->rut == POLL_READY))    //This is unlikely since we just did a nonblocking read
        {
//            std::cout << "FD(" << fd << "): Result is ready" << std::endl;
            pd->rut = nullptr;  //consume the notification and return;
            return;
        }
        //set the state to Waiting
        pd->rut = POLL_WAIT;
    }
    if(flag & UT_IOWRITE)
    {
        if(slowpath(pd->wut == POLL_READY)){
            pd->wut = nullptr;              //consume the notification and return
            return;
        }
        //set the state to parked
        pd->wut = POLL_WAIT;
    }
    uThread* tmp = kThread::currentKT->currentUT;
    pdlock.unlock();

//    MapAndUnlock<int, PollData>* mau = new MapAndUnlock<int,PollData>(&this->IOTable, fd, pd, sndMutex);
    //TODO:decrease the capture list to avoid hitting the hip
    auto lambda([&pd, &tmp, &flag](){
        std::lock_guard<std::mutex> pdlock(pd->mtx);
        if(flag & UT_IOREAD){
            if(pd->rut == POLL_READY)
                tmp->resume();
            else if(pd->rut == POLL_WAIT)
                pd->rut = tmp;
            else
                std::cout << "Exception on rut"<< std::endl;
        }
        if(flag & UT_IOWRITE){
            if(pd->wut == POLL_READY)
                tmp->resume();
            else if(pd->wut == POLL_WAIT)
                pd->wut = tmp;
            else
                std::cout << "Exception on wut"<< std::endl;
        }

    });
    std::function<void()> f(std::cref(lambda));
    kThread::currentKT->currentUT->suspend(f);
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

        if(pd->rut == nullptr || pd->rut == POLL_WAIT)
            pd->rut = POLL_READY;
        else if(pd->rut > POLL_WAIT){
            rut = pd->rut;
            pd->rut = nullptr;
//            std::cout << "Resume UT on fd " << fd << std::endl;
            rut->resume();
        }
        //if ready keep it that way? or should we do sth about it?
    }
    if(flag & UT_IOWRITE){
        if(pd->wut == nullptr || pd->wut == POLL_WAIT)
            pd->wut = POLL_READY;
        else if(pd->wut > POLL_WAIT){
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
    usleep(10000);
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
        assert(res < POLL_CACHE_SIZE);
        std::lock_guard<std::mutex> pdlock(pollCache[res].mtx);
        pollCache[res].newFD = true;
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
