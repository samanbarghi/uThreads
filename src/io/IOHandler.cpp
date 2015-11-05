/*
 * IOHandler.cpp
 *
 *  Created on: Aug 17, 2015
 *      Author: Saman Barghi
 */
#include "IOHandler.h"
#include "Network.h"
#include "kThread.h"
#include "uThread.h"
#include <unistd.h>
#include <sys/types.h>

void IOHandler::open(PollData &pd){
    assert(pd.fd > 0);

    int res = _Open(pd.fd, &pd);
    //TODO: handle epoll errors
}
void IOHandler::wait(PollData& pd, int flag){
    assert(pd.fd > 0);
    if(flag & UT_IOREAD) block(pd, true);
    if(flag & UT_IOWRITE) block(pd, true);

}
void IOHandler::block(PollData &pd, bool isRead){

    std::unique_lock<std::mutex> pdlock(pd.mtx);
    uThread** utp = isRead ? &pd.rut : &pd.wut;
    //TODO:check other states

    if(slowpath(*utp == POLL_READY))    //This is unlikely since we just did a nonblocking read
    {
        *utp = nullptr;  //consume the notification and return;
        return;
    }else if(*utp == nullptr)
            //set the state to Waiting
            *utp = POLL_WAIT;
    else
        std::cout << "Exception on open rut" << std::endl;

    pdlock.unlock();

    uThread* old = kThread::currentKT->currentUT;
//    MapAndUnlock<int, PollData>* mau = new MapAndUnlock<int,PollData>(&this->IOTable, fd, pd, sndMutex);
    //TODO:decrease the capture list to avoid hitting the hip
    auto lambda([&pd, &utp, &old](){
        if(pd.closing) return;
        std::lock_guard<std::mutex> pdlock(pd.mtx);
          if(*utp == POLL_READY)
                old->resume();
            else if(*utp == POLL_WAIT)
                *utp = old;
            else
                std::cout << "Exception on rut"<< std::endl;
    });
    std::function<void()> f(std::cref(lambda));
    kThread::currentKT->currentUT->suspend(f, true); //ask for immediate suspension so the possible closing/notifications do not get lost
//    std::cout << "Wake up from suspension" << std::endl;
    //when epoll returns this ut will be back on readyQueue and pick up from here
}
int IOHandler::close(PollData &pd){

    std::lock_guard<std::mutex> pdlock(pd.mtx);
    assert(pd.rut < POLL_WAIT && pd.wut < POLL_WAIT);
    //remove from underlying poll structure
    int res = _Close(pd.fd);

    pd.reset();
    //TODO: handle epoll errors
    return res;
}

void IOHandler::poll(int timeout, int flag){
    _Poll(timeout);
}

void IOHandler::reset(PollData& pd){
    pd.reset();
}

void IOHandler::unblock(PollData* pd, bool isRead){
    //if closing is set no need to process
    if(pd->closing.load())  return;

    std::lock_guard<std::mutex> pdlock(pd->mtx);
    uThread** ut = isRead ? &pd->rut : &pd->wut;
    uThread* old = *ut;

    if(old == POLL_READY)
        return;
    if(old == nullptr || old == POLL_WAIT)
       *ut = POLL_READY;
    else if(old > POLL_WAIT){
        *ut = nullptr;
        old->resume();
    }

}

void IOHandler::PollReady(PollData* pd, int flag){
    assert(pd != nullptr);

    uThread *rut = nullptr, *wut = nullptr;

    if(flag & UT_IOREAD) unblock(pd, true);
    if(flag & UT_IOWRITE) unblock(pd, false);
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

