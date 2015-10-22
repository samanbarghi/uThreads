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

void IOHandler::block(PollData &pd, int flag){
    assert(pd.fd > 0);

    std::unique_lock<std::mutex> pdlock(pd.mtx);

    //TODO:check other states
    if(flag & UT_IOREAD){
        if(slowpath(pd.rut == POLL_READY))    //This is unlikely since we just did a nonblocking read
        {
//            std::cout << "FD(" << fd << "): Result is ready" << std::endl;
            pd.rut = nullptr;  //consume the notification and return;
            return;
        }else if(pd.rut == nullptr)
                //set the state to Waiting
                pd.rut = POLL_WAIT;
        else
            std::cout << "Exception on open rut" << std::endl;
    }
    if(flag & UT_IOWRITE)
    {
        if(slowpath(pd.wut == POLL_READY)){
            pd.wut = nullptr;              //consume the notification and return
            return;
        }else if(pd.wut == nullptr)
                //set the state to parked
                pd.wut = POLL_WAIT;
        else
            std::cout << "Exception on open wut" << std::endl;
    }
    uThread* tmp = kThread::currentKT->currentUT;
    pdlock.unlock();

//    MapAndUnlock<int, PollData>* mau = new MapAndUnlock<int,PollData>(&this->IOTable, fd, pd, sndMutex);
    //TODO:decrease the capture list to avoid hitting the hip
    auto lambda([&pd, &tmp, &flag](){
        std::lock_guard<std::mutex> pdlock(pd.mtx);
        if(flag & UT_IOREAD){
            if(pd.rut == POLL_READY)
                tmp->resume();
            else if(pd.rut == POLL_WAIT)
                pd.rut = tmp;
            else
                std::cout << "Exception on rut"<< std::endl;
        }
        if(flag & UT_IOWRITE){
            if(pd.wut == POLL_READY)
                tmp->resume();
            else if(pd.wut == POLL_WAIT)
                pd.wut = tmp;
            else
                std::cout << "Exception on wut"<< std::endl;
        }

    });
    std::function<void()> f(std::cref(lambda));
    kThread::currentKT->currentUT->suspend(f);
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
void IOHandler::PollReady(PollData* pd, int flag){
    assert(pd != nullptr);

    uThread *rut = nullptr, *wut = nullptr;
    //if closing is set no need to process
    if(pd->closing.load())
    {
        return;
    }

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

