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

#include "kThread.h"
#include "BlockingSync.h"
#include "MoodyCamelReadyQueue.h"
#include <unistd.h>
#include <sstream>

std::atomic_uint kThread::totalNumberofKTs(0);

__thread kThread* kThread::currentKT = nullptr;
__thread IntrusiveList<uThread>* kThread::ktReadyQueue = nullptr;
__thread ArrayQueue<uThread, KTHREAD_LOCAL_QUEUE_SIZE> *kThread::localQueue                       = nullptr;
__thread moodycamel::ProducerToken *kThread::ptok                                        = nullptr;
__thread moodycamel::ConsumerToken *kThread::ctok                                        = nullptr;

/*
 * This is only called to create defaultKT
 */
kThread::kThread() :
        cv_flag(true), threadSelf() {
    localCluster = &Cluster::defaultCluster;
    initialize();
    initializeMainUT(true);
    uThread::initUT = uThread::createMainUT(Cluster::defaultCluster);
    /*
     * Since this is for defaultKT, current running uThread
     * is the initial one which is responsible for running
     * the main() function.
     */
    currentUT = uThread::initUT;
    initialSynchronization();
}
kThread::kThread(Cluster& cluster, std::function<void(ptr_t)> func, ptr_t args) :
        localCluster(&cluster), threadSelf(&kThread::runWithFunc, this, func,
                args) {
    initialSynchronization();
}

kThread::kThread(Cluster& cluster) :
        localCluster(&cluster), cv_flag(false), threadSelf(&kThread::run, this) {
    initialSynchronization();
}

kThread::~kThread() {
    totalNumberofKTs--;
    localCluster->numberOfkThreads--;

    //free thread local members
    //delete kThread::ktReadyQueue;
    //mainUT->destory(true);
}

void kThread::initialSynchronization() {
    //prevent overflow
    if(totalNumberofKTs + 1 > UINTMAX_MAX)
        exit(EXIT_FAILURE);
    totalNumberofKTs++;
    /*
     * Increase the number of kThreads in the cluster.
     * Since this is always < totalNumberofKTs will
     * not overflow.
     */
    localCluster->numberOfkThreads++;

    //set kernel thread variables
    threadID = std::this_thread::get_id();
}

void kThread::run() {
    initialize();
    initializeMainUT(false);
    defaultRun(this);
}
void kThread::runWithFunc(std::function<void(ptr_t)> func, ptr_t args) {
    initialize();
    //There is no need for a mainUT to be created
    func(args);
}

void kThread::switchContext(uThread* ut, void* args) {
    assert(ut != nullptr);
    assert(ut->stackPointer != 0);
    stackSwitch(ut, args, &kThread::currentKT->currentUT->stackPointer,
            ut->stackPointer, postSwitchFunc);
}

void kThread::switchContext(void* args) {
    uThread* ut = nullptr;
    /*	First check the local queue */
    if (!kThread::localQueue->empty()) {   //If not empty, grab a uThread and run it
        ut = kThread::localQueue->front();
        kThread::localQueue->pop();
    } else {                //If empty try to fill

        ssize_t res = localCluster->tryGetWorks();   //Try to fill the local queue
        kThread::localQueue->updateIndexes(res);
        if (!kThread::localQueue->empty()) {       //If there is more work start using it
            ut = kThread::localQueue->front();
            kThread::localQueue->pop();
        } else {        //If no work is available, Switch to defaultUt
            if ( kThread::currentKT->currentUT->state == uThread::State::YIELD) return; //if the running uThread yielded, continue running it
            ut = mainUT;
        }
    }
    assert(ut != nullptr);
    switchContext(ut, args);
}

void kThread::initialize() {
    /*
     * Set the thread_local pointer to this thread, later we can
     * find the executing thread by referring to this.
     */
    kThread::currentKT = this;
    kThread::ktReadyQueue = new IntrusiveList<uThread>();
    kThread::localQueue = new ArrayQueue<uThread, KTHREAD_LOCAL_QUEUE_SIZE>();
    kThread::ptok = new moodycamel::ProducerToken(localCluster->readyQueue->queue);
    kThread::ctok = new moodycamel::ConsumerToken(localCluster->readyQueue->queue);

}
void kThread::initializeMainUT(bool isDefaultKT) {
    /*
     * if defaultKT, then create a stack for mainUT.
     * kernel thread's stack is assigned to initUT.
     */
    if (slowpath(isDefaultKT)) {
        mainUT = uThread::create(defaultStackSize);
        mainUT->start(*localCluster, (ptr_t) kThread::defaultRun, this, nullptr,
                nullptr);
    } else {
        /*
         * Default function takes up the default kernel thread's
         * stack pointer and run from there
         */
        mainUT = uThread::createMainUT(*localCluster);
    }

    //Default uThreads are not being counted
    uThread::totalNumberofUTs--;
    mainUT->state = uThread::State::READY;
    currentUT = mainUT;
}

void kThread::defaultRun(void* args) {
    kThread* thisKT = (kThread*) args;
    uThread* ut = nullptr;

    size_t count = 0;
    std::stringstream ss;
    while (true) {
        if(kThread::localQueue->empty()){
            count = thisKT->localCluster->getWork();
            std::cout << ss.str();
            //ktReadyQueue should not be empty at this point
            kThread::localQueue->updateIndexes(count);
            assert(!kThread::localQueue->empty());
        }
        ut = kThread::localQueue->front();
        kThread::localQueue->pop();
        //Switch to the new uThread
        thisKT->switchContext(ut, nullptr);
    }
}

void kThread::postSwitchFunc(uThread* nextuThread, void* args = nullptr) {

    kThread* ck = kThread::currentKT;
    //mainUT does not need to be managed here
    if (fastpath(ck->currentUT != kThread::currentKT->mainUT)) {
        switch (ck->currentUT->state) {
        case uThread::State::TERMINATED:
            ck->currentUT->destory(false);
            break;
        case uThread::State::YIELD:
            ck->currentUT->resume();
            ;
            break;
        case uThread::State::MIGRATE:
            ck->currentUT->resume();
            break;
        case uThread::State::WAITING: {
            assert(args != nullptr);
            std::function<void()>* func = (std::function<void()>*) args;
            (*func)();
            break;
        }
        default:
            break;
        }
    }
    //Change the current thread to the next
    ck->currentUT = nextuThread;
    nextuThread->state = uThread::State::RUNNING;
}

//TODO: How can I make this work for defaultKT?
std::thread::native_handle_type kThread::getThreadNativeHandle() {
    if(this != &defaultKT)
        return threadSelf.native_handle();
    else
        return 0;
}

std::thread::id kThread::getID() {
   return threadID;
}
