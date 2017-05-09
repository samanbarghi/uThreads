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

#include "uThread.h"
#include "Cluster.h"
#include "kThread.h"
#include "BlockingSync.h"
#include "uThreadCache.h"
#include "schedulers/Scheduler.h"
#include "io/IOHandler.h"
#include <iostream> //TODO: remove this, add debug object
#include <cassert>

//TODO: change all pointers to unique_ptr or shared_ptr
/***************************************
 *  initialize all static members here
 ***************************************/
std::atomic_ulong uThread::totalNumberofUTs(0);
std::atomic_ulong uThread::uThreadMasterID(1000000);

uThreadCache uThread::utCache;
std::vector<Cluster*> Cluster::clusterList;
Cluster Cluster::defaultCluster;
uThread* uThread::initUT = nullptr;
kThread kThread::defaultKT;
IOHandler IOHandler::iohandler;

/******************************************/
void uThread::reset(){
    stackPointer = (vaddr) this;                //reset stack pointer
    currentCluster = nullptr;
    homekThread = nullptr;
    state = State::INITIALIZED;
    jState = JState::DETACHED;
}

void uThread::invoke(funcvoid3_t func, ptr_t arg1, ptr_t arg2, ptr_t arg3) {
    func(arg1, arg2, arg3);
    currentUThread()->terminate();
    //Context will be switched in kThread
}

void uThread::destory(bool force = false) {
    //check whether we should cache it or not
    totalNumberofUTs--;
    if (slowpath(force) || (utCache.push(this) < 0)) {
        free((ptr_t) (stackBottom));       //Free the allocated memory for stack
    }
}

vaddr uThread::createStack(size_t ssize) {
    ptr_t st = malloc(ssize);
    if (st == nullptr)
        exit(-1); //TODO: Proper exception
    return ((vaddr) st);
}

uThread* uThread::create(size_t ss, bool joinable) {
    uThread* ut = utCache.pop();

    if (ut == nullptr) {
        vaddr mem = uThread::createStack(ss);  //Allocating stack for the thread
        vaddr This = mem + ss - sizeof(uThread);
        ut = new (ptr_t(This)) uThread(mem, ss);
        ut->utvar = new UTVar();
    } else {
        totalNumberofUTs++;
    }
    if(joinable)
        ut->jState = JState::JOINABLE;
    return ut;
}

uThread* uThread::createMainUT(Cluster& cluster) {
    uThread* ut = new uThread(0, 0);
    ut->utvar = new UTVar();
    ut->currentCluster = &cluster;
    return ut;
}

void uThread::start(const Cluster& cluster, ptr_t func, ptr_t args1,
        ptr_t args2, ptr_t args3) {
    currentCluster = const_cast<Cluster*>(&cluster);
    stackPointer = (vaddr) stackInit(stackPointer, (ptr_t) uThread::invoke,
            (ptr_t) func, args1, args2, args3);	//Initialize the thread context
    assert(stackPointer != 0);
    this->resume();
}

void uThread::yield() {
    kThread* ck = kThread::currentkThread();
    assert(ck != nullptr);
    assert(ck->currentUT != nullptr);
    ck->currentUT->state = State::YIELD;
    ck->switchContext();
}

void uThread::migrate(Cluster* cluster) {
    assert(cluster != nullptr);
    if (slowpath(kThread::currentkThread()->localCluster == cluster)) //no need to migrate
        return;
    currentUThread()->currentCluster = cluster;
    currentUThread()->homekThread = cluster->assignkThread();
    currentUThread()->state = State::MIGRATE;
    kThread::currentkThread()->switchContext();
}

void uThread::suspend(funcvoid2_t func, void* args) {
    kThread::currentkThread()->postSuspendFunc = func;
    state = State::WAITING;
    kThread::currentkThread()->switchContext(args);
}

void uThread::resume() {
    if (fastpath(
            state == State::WAITING || state == State::INITIALIZED
                    || state == State::MIGRATE || state == State::YIELD)) {

        state = State::READY;
        if(homekThread == nullptr)
            homekThread = currentCluster->assignkThread();
        Scheduler::schedule(this, *homekThread);//Put uThread back on ReadyQueue
    }
}
void uThread::terminate(){
    if(currentUThread()->jState != JState::DETACHED){
        currentUThread()->waitOrSignal(false);
    }
     currentUThread()->state = State::TERMINATED;
    /*
     * It's scheduler job to switch to another context
     * and terminate this uThread
     */
    kThread::currentkThread()->switchContext();

}
bool uThread::join(){
    /*
     * If Detached, another thread is joining,
     * there is no need to wait here.
     */
    if(jState != JState::JOINABLE) return false;
    jState = JState::JOINING;
    waitOrSignal(true);
    return true;
}

void uThread::detach(){
    jState = JState::DETACHED;
    joinMtx.acquire();
    if(!joinWait.empty()){
        joinWait.signal(joinMtx);
    }
    joinMtx.release();
}

uThread* uThread::currentUThread() {
    return kThread::currentKT->currentUT;
}
