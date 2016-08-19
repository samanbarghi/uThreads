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

#include <runtime/schedulers/Scheduler.h>
#include "IOHandler.h"
#include "Network.h"
#include "runtime/kThread.h"
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <sstream>
#include <time.h>

IOHandler::IOHandler(): unblockCounter(0),
                        ioKT(Cluster::defaultCluster, &IOHandler::pollerFunc, (ptr_t)this),
                        poller(*this), isPolling(ATOMIC_FLAG_INIT){}

void IOHandler::open(PollData &pd){
    assert(pd.fd > 0);
    bool expected = false;

    //If another uThread called opened already, return
    //TODO: use a mutex instead?
    if(!__atomic_compare_exchange_n(&pd.opened, &expected, true, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return;

    //Add the file descriptor to the poller struct
    int res = poller._Open(pd.fd, pd);
    if(res != 0){
        __atomic_exchange_n(&pd.opened, false, __ATOMIC_RELAXED);
        std::cerr << "EPOLL_ERROR: " << errno << std::endl;
        //TODO: this should be changed to an exception
        exit(EXIT_FAILURE);
    }
    //TODO: handle epoll errors
}
void IOHandler::wait(PollData& pd, int flag){
    assert(pd.fd > 0);
    if(flag & Flag::UT_IOREAD) block(pd, true);
    if(flag & Flag::UT_IOWRITE) block(pd, false);
}
void IOHandler::block(PollData &pd, bool isRead){

    if(!pd.opened) open(pd);
    uThread** utp = isRead ? &pd.rut : &pd.wut;

    uThread* ut = *utp;

    if(ut == POLL_READY)
    {
        //No need for atomic, as for now only a single uThread can block on
        //read/write for each fd
        *utp = nullptr;  //consume the notification and return;
        return;
    }else if(ut > POLL_WAIT)
        std::cerr << "Exception on open rut" << std::endl;

    //This does not need synchronization, since only a single thread
    //will access it before and after blocking
    pd.isBlockingOnRead = isRead;

    kThread::currentKT->currentUT->suspend((funcvoid2_t)IOHandler::postSwitchFunc, (void*)&pd);
    //ask for immediate suspension so the possible closing/notifications do not get lost
    //when epoll returns this ut will be back on readyQueue and pick up from here
}
void IOHandler::postSwitchFunc(void* ut, void* args){
    assert(args != nullptr);
    assert(ut != nullptr);

    uThread* utold = (uThread*)ut;
    PollData* pd = (PollData*) args;
    if(pd->closing) return;
    uThread** utp = pd->isBlockingOnRead ? &pd->rut : &pd->wut;

    uThread *old, *expected ;
    while(true){
        old = *utp;
        expected = nullptr;
        if(old == POLL_READY){
            *utp = nullptr;         //consume the notification and resume
            utold->resume();
            return;
        }
        if(old != nullptr)
            std::cerr << "Exception on rut"<< std::endl;

        if(__atomic_compare_exchange_n(utp, &expected, utold, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED ))
            break;
    }
}
int IOHandler::close(PollData &pd){

    //unblock pd if blocked
    if(pd.rut > POLL_WAIT)
        unblock(pd, true);
    if(pd.wut > POLL_WAIT);
        unblock(pd, true);

    bool expected = false;
    //another thread is already closing this fd
    if(!__atomic_compare_exchange_n(&pd.closing, &expected, true, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED))
        return -1;
    //remove from underlying poll structure
    int res = poller._Close(pd.fd);

    //pd.reset();
    //TODO: handle epoll errors
    pd.reset();
    pollCache.pushPollData(&pd);
    return res;
}

ssize_t IOHandler::poll(int timeout, int flag){
    unblockCounter=0;
    if( poller._Poll(timeout) < 0) return -1;
#ifndef NPOLLBULKPUSH
    if(unblockCounter >0){
        //Bulk push everything to the related cluster ready Queue
        Scheduler::bulkPush();
    }
#endif //NPOLLBULKPUSH
    return unblockCounter;
}

void IOHandler::reset(PollData& pd){
    pd.reset();
}

bool IOHandler::unblock(PollData &pd, bool isRead){

    //if it's closing no need to process
    if(pd.closing) return false;

    uThread** utp = isRead ? &pd.rut : &pd.wut;
    uThread* old;

    while(true){
        old = *utp;
        if(old == POLL_READY) return false;
        //For now only if io is ready we call the unblock
        uThread* utnew = nullptr;
        if(old == nullptr || old == POLL_WAIT) utnew = POLL_READY;
        if(__atomic_compare_exchange_n(utp, &old, utnew, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)){
            if(old > POLL_WAIT){
#ifdef NPOLLBULKPUSH
                old->resume();
#else
                Scheduler::prepareBulkPush(old);
#endif //NPOLLBULKPUSH
                return true;
            }
            break;
        }
    }
    return false;
}

void IOHandler::PollReady(PollData &pd, int flag){
    if( (flag & Flag::UT_IOREAD) && unblock(pd, true)) unblockCounter++;
    if( (flag & Flag::UT_IOWRITE) && unblock(pd, false)) unblockCounter++;
}

ssize_t IOHandler::nonblockingPoll(){
    ssize_t counter = -1;

#ifndef NPOLLNONBLOCKING
    if(!isPolling.test_and_set(std::memory_order_acquire)){
        //do a nonblocking poll
        counter = poll(0,0);
        isPolling.clear(std::memory_order_release);
    }
#endif //NPOLLNONBLOCKING
    return counter;
}

void IOHandler::pollerFunc(void* ioh){
    IOHandler* cioh = (IOHandler*)ioh;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    const long BILLION = 1000000000;
    const long MS = 1000000;

    while(true){

       /*
        * This sequence of wait and post makes sure the poller thread
        * only polls if  there is a blocked kThread, otherwise it waits
        * on the semaphore. This works along with post and wait in
        * the scheduler.
        */

       if( !(cioh->sem.timedwait(ts)) )
            cioh->sem.post();
       else{
            ts.tv_nsec = ts.tv_nsec + MS;
            if slowpath(ts.tv_nsec >= BILLION) {
                ts.tv_sec = ts.tv_sec + 1;
                ts.tv_nsec = ts.tv_nsec - BILLION;
            }
       }

#if defined(NPOLLNONBLOCKING)
        //do a blocking poll
        cioh->poll(-1, 0);
#else
        if(!cioh->isPolling.test_and_set(std::memory_order_acquire)){
            //do a blocking poll
            cioh->poll(-1, 0);
            cioh->isPolling.clear(std::memory_order_release);
        }
#endif //NPOLLNONBLOCKING

  }
}
