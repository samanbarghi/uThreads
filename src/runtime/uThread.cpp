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
#include <iostream> //TODO: remove this, add debug object
#include <cassert>

//TODO: change all pointers to unique_ptr or shared_ptr
/***************************************
 *  initialize all static members here
 ***************************************/
std::atomic_ulong uThread::totalNumberofUTs(0);
std::atomic_ulong uThread::uThreadMasterID(0);

uThreadCache uThread::utCache;
Cluster Cluster::defaultCluster;
uThread* uThread::initUT = nullptr;
kThread kThread::defaultKT;

/******************************************/
void uThread::reset() {
    stackPointer = (vaddr) this;                //reset stack pointer
    currentCluster = nullptr;
    state = State::INITIALIZED;
}

void uThread::destory(bool force = false) {
    //check whether we should cache it or not
    totalNumberofUTs--;
    if (slowpath(force) || (utCache.push(this) < 0)) {
        munmap((ptr_t) (stackBottom), stackSize);       //Free the allocated memory for stack
    }
}

vaddr uThread::createStack(size_t ssize) {
#if defined(__linux__)
    ptr_t st = mmap(nullptr,ssize, PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE | MAP_GROWSDOWN, -1, 0);
    if(st == MAP_FAILED)
    {
        std::cerr << "Error allocating memory for stack" << std::endl;
        exit(1);
    }
#else
#error undefined platform: only __linux__ supported at this time
#endif
    return ((vaddr) st);
}

uThread* uThread::create(size_t ss) {
    uThread* ut = utCache.pop();

    if (ut == nullptr) {
        vaddr mem = uThread::createStack(ss);  //Allocating stack for the thread
        vaddr This = mem + ss - sizeof(uThread);
        ut = new (ptr_t(This)) uThread(mem, ss);
    } else {
        totalNumberofUTs++;
    }
    return ut;

}

uThread* uThread::createMainUT(Cluster& cluster) {
    uThread* ut = new uThread(0, 0);
    ut->currentCluster = &cluster;
    return ut;
}

void uThread::start(const Cluster& cluster, ptr_t func, ptr_t args1,
        ptr_t args2, ptr_t args3) {
    currentCluster = const_cast<Cluster*>(&cluster);
    stackPointer = (vaddr) stackInit(stackPointer, (ptr_t) Cluster::invoke,
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
    currentUThread()->state = State::MIGRATE;
    kThread::currentkThread()->switchContext();
}

void uThread::suspend(std::function<void()>& func) {
    state = State::WAITING;
    kThread::currentkThread()->switchContext(&func);
}

void uThread::resume() {
    if (fastpath(
            state == State::WAITING || state == State::INITIALIZED
                    || state == State::MIGRATE || state == State::YIELD)) {

        state = State::READY;
        currentCluster->schedule(this);//Put uThread back on ReadyQueue
    }
}

void uThread::terminate() {
    currentUThread()->state = State::TERMINATED;
    /*
     * It's scheduler job to switch to another context
     * and terminate this uThread
     */
    kThread::currentkThread()->switchContext();
}

uThread* uThread::currentUThread() {
    return kThread::currentKT->currentUT;
}
