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

using uThreads::runtime::uThread;
using uThreads::runtime::kThread;
using uThreads::runtime::Cluster;
using uThreads::runtime::KTLocal;

std::atomic_uint kThread::totalNumberofKTs(0);

__thread kThread *kThread::currentKT = nullptr;
__thread KTLocal *kThread::ktlocal = nullptr;
__thread funcvoid2_t kThread::postSuspendFunc = nullptr;

/*
 * This is only called to create defaultKT
 */
kThread::kThread() :
        threadSelf(),
        ktvar(new KTVar()),
        scheduler(Scheduler::getScheduler(Cluster::defaultCluster)){
    localCluster = &Cluster::defaultCluster;
    iohandler = IOHandler::getkThreadIOHandler(*this);
    initialize();
    initializeMainUT(true);
    uThread::initUT = uThread::createMainUT(Cluster::defaultCluster);
    uThread::initUT->homekThread = this;
    /*
     * Add the kThread to the list of kThreads in the Cluster
     */
    localCluster->addNewkThread(*this);
    /*
     * Since this is for defaultKT, current running uThread
     * is the initial one which is responsible for running
     * the main() function.
     */
    currentUT = uThread::initUT;
    initialSynchronization();
}

kThread::kThread(const Cluster &cluster, std::function<void(ptr_t)> func, ptr_t args) :
        localCluster((Cluster*)&cluster), ktvar(new KTVar()),
        scheduler(Scheduler::getScheduler(cluster)),
        threadSelf(&kThread::runWithFunc, this, func,
                   args) {
    threadSelf.detach();
    initialSynchronization();
}

kThread::kThread(const Cluster &cluster) :
        localCluster((Cluster*)&cluster), ktvar(new KTVar()),
        scheduler(Scheduler::getScheduler(cluster)),
        iohandler(IOHandler::getkThreadIOHandler(*this)),
        threadSelf(&kThread::run, this) {
    threadSelf.detach();
    /*
     * Add the kThread to the list of kThreads in the Cluster
     */
    localCluster->addNewkThread(*this);
    initialSynchronization();
}

kThread::~kThread() {
    totalNumberofKTs--;
    localCluster->numberOfkThreads--;

    // free thread local members
}

void kThread::initialSynchronization() {
    // prevent overflow
    if (totalNumberofKTs + 1 > UINTMAX_MAX)
        exit(EXIT_FAILURE);
    totalNumberofKTs++;

    // set kernel thread variables
    threadID = (this == &defaultKT) ? std::this_thread::get_id() : this->threadSelf.get_id();
}

void kThread::run() {
    initialize();
    initializeMainUT(false);
    defaultRun(this);
}

void kThread::runWithFunc(std::function<void(ptr_t)> func, ptr_t args) {
    // There is no need for a mainUT to be created
    func(args);
}

void kThread::switchContext(uThread *ut, void *args) {
    assert(ut != nullptr);
    assert(ut->stackPointer != 0);
    stackSwitch(ut, args, &kThread::currentKT->currentUT->stackPointer,
                ut->stackPointer, postSwitchFunc);
}

void kThread::switchContext(void *args) {

    uThread *ut = scheduler->nonBlockingSwitch(*this);
    if (ut == nullptr)
        return;
    switchContext(ut, args);
}

void kThread::initialize() {
    /*
     * Set the thread_local pointer to this thread, later we can
     * find the executing thread by referring to this.
     */
    kThread::currentKT = this;

    // Initialize kt local vars
    kThread::ktlocal = new KTLocal();
}

void kThread::initializeMainUT(bool isDefaultKT) {
    /*
     * if defaultKT, then create a stack for mainUT.
     * kernel thread's stack is assigned to initUT.
     */
    if (slowpath(isDefaultKT)) {
        mainUT = uThread::create(defaultStackSize);
        /*
         * can't use mainUT->start as mainUT should not end up in
         * the Ready Queue. Thus, the stack pointer should be initiated
         * directly.
         */
        mainUT->stackPointer = (vaddr) stackInit(mainUT->stackPointer,
                                                 (ptr_t) uThread::invoke,
                                                 (ptr_t) kThread::defaultRun,
                                                 (void *) this, nullptr,
                                                 nullptr);
        mainUT->state = uThread::State::READY;
    } else {
        /*
         * Default function takes up the default kernel thread's
         * stack pointer and run from there
         */
        mainUT = uThread::createMainUT(*localCluster);
        currentUT = mainUT;
        mainUT->state = uThread::State::RUNNING;
    }

    // Default uThreads are not being counted
    uThread::totalNumberofUTs--;
}

void kThread::defaultRun(void *args) {
    kThread *thisKT = (kThread *) args;
    uThread *ut = nullptr;

    while (true) {
        ut = thisKT->scheduler->blockingSwitch(*thisKT);
        // This should never happen
        if (ut == nullptr) continue;
        // Switch to the new uThread
        thisKT->switchContext(ut, nullptr);
    }
}

void kThread::postSwitchFunc(uThread *nextuThread, void *args = nullptr) {

    kThread *ck = kThread::currentKT;
    // mainUT does not need to be managed here
    if (fastpath(ck->currentUT != kThread::currentKT->mainUT)) {
        switch (ck->currentUT->state) {
            case uThread::State::TERMINATED:
                ck->currentUT->destroy(false);
                break;
            case uThread::State::YIELD:
                ck->currentUT->resume();;
                break;
            case uThread::State::MIGRATE:
                ck->currentUT->resume();
                break;
            case uThread::State::WAITING: {
                // function and the argument should be set for pss
                assert(postSuspendFunc != nullptr);
                postSuspendFunc((void *) ck->currentUT, args);
                break;
            }
            default:
                break;
        }
    }
    // Change the current thread to the next
    ck->currentUT = nextuThread;
    nextuThread->state = uThread::State::RUNNING;
}

// TODO(saman): How can I make this work for defaultKT?
std::thread::native_handle_type kThread::getThreadNativeHandle() {
    if (this != &defaultKT)
        return threadSelf.native_handle();
    else
        return 0;
}

std::thread::id kThread::getID() {
    return threadID;
}
