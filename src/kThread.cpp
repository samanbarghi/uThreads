/*
 * kThread.cpp
 *
 *  Created on: Oct 27, 2014
 *      Author: Saman Barghi
 */

#include "kThread.h"
#include "BlockingSync.h"
#include <iostream>
#include <unistd.h>

mword kThread::totalNumberofKTs = 0;
std::mutex kThread::kThreadSyncLock;


__thread kThread* kThread::currentKT 					= nullptr;
__thread EmbeddedList<uThread>* kThread::ktReadyQueue 	= nullptr;
IOHandler* kThread::ioHandler 					= nullptr;
//__thread uThread* kThread::currentUT = nullptr;


kThread::kThread(bool initial) : shouldSpin(true){								//This is only for initial kThread
	this->threadSelf = new std::thread();										//For default kThread threadSelf should be initialized to current thread
	this->localCluster = Cluster::defaultCluster;
	this->initialize(true);

	assert(uThread::initUT != nullptr);
	this->currentUT = uThread::initUT;											//Current running uThread is the initial one
//	kThread::ioHandler = newIOHandler();
	initialSynchronization();
}

kThread::kThread(Cluster* cluster) : localCluster(cluster), shouldSpin(true){
	threadSelf = new std::thread(&kThread::run, this);
//	kThread::ioHandler = newIOHandler();
	initialSynchronization();
}

kThread::kThread() : localCluster(Cluster::defaultCluster), shouldSpin(true){
	threadSelf = new std::thread(&kThread::run, this);
//	kThread::ioHandler = newIOHandler();
	initialSynchronization();
}

kThread::~kThread() {
	if(threadSelf->joinable())
		threadSelf->join();															//wait for the thread to terminate properly

	std::lock_guard<std::mutex> lock(kThreadSyncLock);
	totalNumberofKTs--;
	localCluster->numberOfkThreads--;

	//free thread local members
	delete kThread::ktReadyQueue;
	//free(kThread::ioHandler);
}

void kThread::initialSynchronization(){
	std::lock_guard<std::mutex> lock(kThreadSyncLock);
	totalNumberofKTs++;
	localCluster->numberOfkThreads++;											//Increas the number of kThreads in the cluster
	//std::cout << "This Cluster " << localCluster->getClusterID() << " had " << localCluster->numberOfkThreads << std::endl;
}

IOHandler* kThread::newIOHandler(){
    IOHandler* ioh = new EpollIOHandler();                                  //TODO: have a default value and possibility to change for iohandler.
    return ioh;
}

void kThread::run() {
	this->initialize(false);
//	std::cout << "Started Running! ID: " << std::this_thread::get_id() << std::endl;
	defaultRun(this);
}

void kThread::switchContext(uThread* ut,void* args) {
//	std::cout << "SwitchContext: " << kThread::currentKT->currentUT << " TO: " << ut << " IN: " << std::this_thread::get_id() << std::endl;
    assert(ut != nullptr);
    assert(ut->stackPointer != nullptr);

	stackSwitch(ut, args, &kThread::currentKT->currentUT->stackPointer, ut->stackPointer, postSwitchFunc);
}

void kThread::switchContext(void* args){
	uThread* ut = nullptr;
	/*	First check the internal queue */
    EmbeddedList<uThread>* ktrq = ktReadyQueue;
	if(!ktrq->empty()){										//If not empty, grab a uThread and run it
		ut = ktrq->front();
		ktrq->pop_front();
	}else{													//If empty try to fill
		localCluster->tryGetWorks(ktrq);							//Try to fill the local queue
		if(!ktrq->empty()){									//If there is more work start using it
			ut = ktrq->front();
			ktrq->pop_front();
		}else{												//If no work is available, Switch to defaultUt
			if(kThread::currentKT->currentUT->status == YIELD)	return;				//if the running uThread yielded, continue running it
			ut = mainUT;
		}
	}
	assert(ut != nullptr);
//	std::cout << "SwitchContext: " << kThread::currentKT->currentUT<< " Status: " << kThread::currentKT->currentUT->status <<   " IN Thread:" << std::this_thread::get_id()   << std::endl;

//	else{std::cout << std::endl << std::this_thread::get_id() << ":Got WORK:" << ut->id << std::endl;}

	switchContext(ut,args);
}

void kThread::initialize(bool isDefaultKT) {
	kThread::currentKT		=	this;											//Set the thread_locl pointer to this thread, later we can find the executing thread by referring to this
	kThread::ktReadyQueue = new EmbeddedList<uThread>();

	if(isDefaultKT)
	    this->mainUT = new uThread((funcvoid1_t)kThread::defaultRun, this, default_uthread_priority, this->localCluster); //if defaultKT, then create a stack for mainUT cause pthread stack is assigned to initUT
	else
	    this->mainUT = new uThread(this->localCluster);			                    //Default function takes up the default pthread's stack pointer and run from there


	uThread::decrementTotalNumberofUTs();										//Default uThreads are not counted as valid work
	this->mainUT->status	=	READY;
	this->currentUT		= 	this->mainUT;
	//std::cout << this->getThreadID() << std::endl;
}

void kThread::defaultRun(void* args) {
    kThread* thisKT = (kThread*) args;
    uThread* ut = nullptr;

    while (true) {
        //TODO: break this loop when total number of uThreads are less than 1, and end the kThread
        thisKT->localCluster->getWork(thisKT->ktReadyQueue);
        assert(!ktReadyQueue->empty());                         //ktReadyQueue should not be empty at this point
        ut = thisKT->ktReadyQueue->front();
        thisKT->ktReadyQueue->pop_front();

        thisKT->switchContext(ut, nullptr);                     //Switch to the new uThread
    }
}

void kThread::postSwitchFunc(uThread* nextuThread, void* args=nullptr) {
//	std::cout << "This is post func for: " << kThread::currentKT->currentUT << " With status: " << kThread::currentKT->currentUT->status << std:: endl;

    kThread* ck = kThread::currentKT;
	if(ck->currentUT != kThread::currentKT->mainUT){			//DefaultUThread do not need to be managed here
		switch (ck->currentUT->status) {
			case TERMINATED:
				ck->currentUT->terminate();
				break;
			case YIELD:
				ck->localCluster->readyQueue.push(kThread::currentKT->currentUT);
				break;
			case MIGRATE:
				ck->currentUT->currentCluster->uThreadSchedule(kThread::currentKT->currentUT);
				break;
			case WAITING:
			{
			    assert(args != nullptr);
			    std::function<void()>* func = (std::function<void()>*)args;
			    (*func)();
				break;
			}
			default:
				break;
		}
	}
//	std::cout << "This is the next thread: " << nextuThread << " : ID: " << nextuThread->getId() <<  " Stack: " << nextuThread->stackBottom << " Next:" << nextuThread->next  << " Pointer: " << nextuThread->stackPointer << std::endl;
	ck->currentUT	= nextuThread;								//Change the current thread to the next
	nextuThread->status = RUNNING;
}

//TODO: Get rid of this. For test purposes only
void kThread::printThreadId(){
	std::thread::id main_thread_id = std::this_thread::get_id();
//	std::cout << "This is my thread id: " << main_thread_id << std::endl;
}

std::thread::native_handle_type kThread::getThreadNativeHandle() {
	assert(this->threadSelf != nullptr);
	return this->threadSelf->native_handle();
}

std::thread::id kThread::getThreadID() {
	assert(this->threadSelf != nullptr);
	return this->threadSelf->get_id();
}
void kThread::setShouldSpin(bool spin){
    this->shouldSpin = spin;
}
