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

__thread kThread* kThread::currentKT = nullptr;
//__thread uThread* kThread::currentUT = nullptr;
__thread EmbeddedList<uThread>* kThread::ktReadyQueue = nullptr;


kThread* kThread::defaultKT = new kThread(true);
//kThread* kThread::syscallKT = new kThread(&Cluster::syscallCluster);

kThread::kThread(bool initial) : localCluster(&Cluster::defaultCluster){		//This is only for initial kThread
	//threadSelf does not need to be initialized
	this->initialize();
	currentUT = uThread::initUT;												//Current running uThread is the initial one
	totalNumberofKTs++;
	localCluster->numberOfkThreads++;											//Increas the number of kThreads in the cluster
}

kThread::kThread(Cluster* cluster) : localCluster(cluster){
	totalNumberofKTs++;
	threadSelf = new std::thread(&kThread::run, this);
	localCluster->numberOfkThreads++;
}

kThread::kThread() : localCluster(&Cluster::defaultCluster){
	totalNumberofKTs++;
	threadSelf = new std::thread(&kThread::run, this);
	localCluster->numberOfkThreads++;
}

kThread::~kThread() {
	threadSelf->join();															//wait for the thread to terminate properly
	totalNumberofKTs--;
	localCluster->numberOfkThreads--;
}

void kThread::run() {
	this->initialize();
//	std::cout << "Started Running! ID: " << std::this_thread::get_id() << std::endl;
	switchContext(mainUT);
}

void kThread::switchContext(uThread* ut,void* args = nullptr) {
//	std::cout << "SwitchContext: " << kThread::currentKT->currentUT << " TO: " << ut << " IN: " << std::this_thread::get_id() << std::endl;
	stackSwitch(ut, args, &kThread::currentKT->currentUT->stackPointer, ut->stackPointer, postSwitchFunc);
}

void kThread::switchContext(void* args = nullptr){
	uThread* ut = nullptr;
	/*	First check the internal queue */
	if(!ktReadyQueue->empty()){														//If not empty, grab a uThread and run it
		ut = ktReadyQueue->front();
		ktReadyQueue->pop_front();
	}else{																			//If empty try to fill
		localCluster->tryGetWorks(ktReadyQueue);									//Try to fill the local queue
		if(!ktReadyQueue->empty()){													//If there is more work start using it
			ut = ktReadyQueue->front();
			ktReadyQueue->pop_front();
		}else{																		//If no work is available, Switch to defaultUt
			if(kThread::currentKT->currentUT->status == YIELD)	return;				//if the running uThread yielded, continue running it
			//spin for a while before blocking
			for(int i=00; i > 0 ; i--){
				if(localCluster->readyQueue.size > 0){
					//Get work and break;
					ut = localCluster->tryGetWork();
					std::cout << "Got Spin and work" << std::endl;
					break;
				}
				for (int j=1000; j>0; j--)
					__asm("");
			}
			if(ut == nullptr)
				ut = mainUT;															//otherwise, pick the mainUT and run it
		}
	}
//	std::cout << "SwitchContext: " << kThread::currentKT->currentUT<< " Status: " << kThread::currentKT->currentUT->status <<   " IN Thread:" << std::this_thread::get_id()   << std::endl;

//	else{std::cout << std::endl << std::this_thread::get_id() << ":Got WORK:" << ut->id << std::endl;}
	switchContext(ut,args);
}

void kThread::initialize() {
	kThread::currentKT		=	this;											//Set the thread_locl pointer to this thread, later we can find the executing thread by referring to this
	kThread::ktReadyQueue = new EmbeddedList<uThread>();

	this->mainUT = new uThread((funcvoid1_t)kThread::defaultRun, this, default_uthread_priority, this->localCluster);			//Default function should not be put back on readyQueue, or be scheduled by cluster
	uThread::totalNumberofUTs--;												//Default uThreads are not counted as valid work
	this->mainUT->status	=	READY;
	this->currentUT		= 	this->mainUT;

	char* path;
}

void kThread::defaultRun(void* args) {
	kThread* thisKT = (kThread*) args;
	while(true){
		//TODO: break this loop when total number of uThreads are less than 1, and end the kThread
//		std::cout << "Sleep on ReadyQueue" << std::endl;
		uThread* ut = thisKT->localCluster->getWork();
		thisKT->switchContext(ut);
	}
}

void kThread::postSwitchFunc(uThread* nextuThread, void* args=nullptr) {
//	std::cout << "This is post func for: " << kThread::currentKT->currentUT << " With status: " << kThread::currentKT->currentUT->status << std:: endl;

	if(kThread::currentKT->currentUT != kThread::currentKT->mainUT){			//DefaultUThread do not need to be managed here
		switch (kThread::currentKT->currentUT->status) {
			case TERMINATED:
				kThread::currentKT->currentUT->terminate();
				break;
			case YIELD:
				kThread::currentKT->localCluster->readyQueue.push(kThread::currentKT->currentUT);
				break;
			case MIGRATE:
				kThread::currentKT->currentUT->currentCluster->uThreadSchedule(kThread::currentKT->currentUT);
				break;
			case WAITING:
			{
				QueueAndLock* qal = (QueueAndLock*)args;
				qal->list->push_back(kThread::currentKT->currentUT);
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
	kThread::currentKT->currentUT	= nextuThread;								//Change the current thread to the next
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
