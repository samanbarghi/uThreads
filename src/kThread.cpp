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

__thread kThread* kThread::currentKT = nullptr;
//__thread uThread* kThread::currentUT = nullptr;
__thread EmbeddedList<uThread>* kThread::ktReadyQueue = nullptr;


kThread* kThread::defaultKT = new kThread(true);
//kThread* kThread::syscallKT = new kThread(&Cluster::syscallCluster);

kThread::kThread(bool initial) : localCluster(&Cluster::defaultCluster), shouldSpin(true){		//This is only for initial kThread
	//threadSelf does not need to be initialized
	this->initialize();
	currentUT = uThread::initUT;												//Current running uThread is the initial one
	initialSynchronization();
}

kThread::kThread(Cluster* cluster) : localCluster(cluster), shouldSpin(true){
	threadSelf = new std::thread(&kThread::run, this);
	initialSynchronization();
}

kThread::kThread() : localCluster(&Cluster::defaultCluster), shouldSpin(true){
	threadSelf = new std::thread(&kThread::run, this);
	initialSynchronization();
}

kThread::~kThread() {
	threadSelf->join();															//wait for the thread to terminate properly
	std::lock_guard<std::mutex> lock(kThreadSyncLock);
	totalNumberofKTs--;
	localCluster->numberOfkThreads--;
}

void kThread::initialSynchronization(){
	std::lock_guard<std::mutex> lock(kThreadSyncLock);
	totalNumberofKTs++;
	localCluster->numberOfkThreads++;											//Increas the number of kThreads in the cluster
}

void kThread::run() {
	this->initialize();
//	std::cout << "Started Running! ID: " << std::this_thread::get_id() << std::endl;
	defaultRun(this);
}

void kThread::switchContext(uThread* ut,void* args) {
//	std::cout << "SwitchContext: " << kThread::currentKT->currentUT << " TO: " << ut << " IN: " << std::this_thread::get_id() << std::endl;
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
//	std::cout << "SwitchContext: " << kThread::currentKT->currentUT<< " Status: " << kThread::currentKT->currentUT->status <<   " IN Thread:" << std::this_thread::get_id()   << std::endl;

//	else{std::cout << std::endl << std::this_thread::get_id() << ":Got WORK:" << ut->id << std::endl;}
	switchContext(ut,args);
}

void kThread::initialize() {
	kThread::currentKT		=	this;											//Set the thread_locl pointer to this thread, later we can find the executing thread by referring to this
	kThread::ktReadyQueue = new EmbeddedList<uThread>();

	this->mainUT = new uThread(this->localCluster);			                    //Default function takes up the default pthread's stack pointer and run from there
	uThread::decrementTotalNumberofUTs();										//Default uThreads are not counted as valid work
	this->mainUT->status	=	READY;
	this->currentUT		= 	this->mainUT;
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
				QueueAndLock* qal = (QueueAndLock*)args;
				qal->list->push_back(ck->currentUT);
				if(qal->mutex)
					qal->mutex->unlock();
				else if(qal->umutex)
					qal->umutex->release();

				delete(qal);
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
	return this->threadSelf->native_handle();
}

std::thread::id kThread::getThreadID() {
	return this->threadSelf->get_id();
}
void kThread::setShouldSpin(bool spin){
    this->shouldSpin = spin;
}
