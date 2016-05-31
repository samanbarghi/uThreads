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

IOHandler::IOHandler(Cluster& cluster): bulkCounter(0), localCluster(&cluster), ioKT(cluster, &IOHandler::pollerFunc, (ptr_t)this), poller(*this){}

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

    if(slowpath(ut == POLL_READY))    //This is unlikely since we just did a nonblocking read
    {
        //No need for atomic, as for now only a single uThread can block on
        //read for each fd
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

void IOHandler::poll(int timeout, int flag){
    poller._Poll(timeout);
}

void IOHandler::reset(PollData& pd){
    pd.reset();
}

void IOHandler::unblock(PollData &pd, bool isRead){

    //if it's closing no need to process
    if(pd.closing) return;

    uThread** utp = isRead ? &pd.rut : &pd.wut;
    uThread* old;

    while(true){
        old = *utp;
        if(old == POLL_READY) return;
        //For now only if io is ready we call the unblock
        uThread* utnew = nullptr;
        if(old == nullptr || old == POLL_WAIT) utnew = POLL_READY;
        if(__atomic_compare_exchange_n(utp, &old, utnew, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)){
            if(old > POLL_WAIT)
                old->resume();

            break;
        }
    }
}

void IOHandler::unblockBulk(PollData &pd, bool isRead){

    //if it's closing no need to process
    if(slowpath(pd.closing)) return;

    uThread** utp = isRead ? &pd.rut : &pd.wut;
    uThread* old;

    while(true){
        old = *utp;
        if(old == POLL_READY) return;
        //For now only if io is ready we call the unblock
        uThread* utnew = nullptr;
        if(old == nullptr || old == POLL_WAIT) utnew = POLL_READY;
        if(__atomic_compare_exchange_n(utp, &old, utnew, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)){
            if(old > POLL_WAIT){
                old->state = uThread::State::READY;
                Scheduler::prepareBulkPush(old);
                bulkCounter++;
            }
            break;
        }
    }
    /* It is the responsibility of the caller function
     * to call scheduleMany to schedule the piled-up
     * uThreads on the related cluster.
     */
}

void IOHandler::PollReady(PollData &pd, int flag){
    if(flag & Flag::UT_IOREAD) unblock(pd, true);
    if(flag & Flag::UT_IOWRITE) unblock(pd, false);
}
void IOHandler::PollReadyBulk(PollData &pd, int flag, bool isLast){
    if(flag & Flag::UT_IOREAD) unblockBulk(pd, true);
    if(flag & Flag::UT_IOWRITE) unblockBulk(pd, false);
    //if this is the last item return by the poller
    //Bulk push everything to the related cluster ready Queue
    if(slowpath(isLast) && bulkCounter >0){
        Scheduler::bulkPush(*localCluster);
        bulkCounter=0;
    }
}

void IOHandler::pollerFunc(void* ioh){
    IOHandler* cioh = (IOHandler*)ioh;
    while(true){
       cioh->poll(-1, 0);
   }
}
