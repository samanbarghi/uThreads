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

std::atomic_uint kThread::totalNumberofKTs(0);

__thread kThread* kThread::currentKT                    = nullptr;
__thread IntrusiveList<uThread>* kThread::ktReadyQueue  = nullptr;

IOHandler* kThread::ioHandler                           = nullptr;

//__thread uThread* kThread::currentUT = nullptr;


kThread::kThread(bool initial) : cv_flag(true){       //This is only for initial kThread
	threadSelf = new std::thread();                                     //For default kThread threadSelf should be initialized to current thread
	localCluster = &Cluster::defaultCluster;
	initialize(true);
	uThread::initUT = uThread::createMainUT(Cluster::defaultCluster);
	currentUT = uThread::initUT;                                        //Current running uThread is the initial one

	kThread::ioHandler = IOHandler::createIOHandler();
	initialSynchronization();
}

kThread::kThread(Cluster& cluster) : localCluster(&cluster), cv_flag(false){
	threadSelf = new std::thread(&kThread::run, this);
	threadSelf->detach();                                                       //Detach the thread from the running thread
	initialSynchronization();
}

kThread::kThread() : localCluster(&Cluster::defaultCluster){
	threadSelf = new std::thread(&kThread::run, this);
	threadSelf->detach();                                                       //Detach the thread from the running thread
	initialSynchronization();
}

kThread::~kThread() {
	totalNumberofKTs--;
	localCluster->numberOfkThreads--;

	//free thread local members
	free(kThread::ioHandler);
	delete kThread::ktReadyQueue;
	mainUT->destory(true);
}

void kThread::initialSynchronization(){
	totalNumberofKTs++;
	localCluster->numberOfkThreads++;											//Increas the number of kThreads in the cluster
}

void kThread::run() {
	initialize(false);
	defaultRun(this);
}

void kThread::switchContext(uThread* ut,void* args) {
    assert(ut != nullptr);
    assert(ut->stackPointer != 0);
	stackSwitch(ut, args, &kThread::currentKT->currentUT->stackPointer, ut->stackPointer, postSwitchFunc);
}

void kThread::switchContext(void* args){
	uThread* ut = nullptr;
	/*	First check the internal queue */
    IntrusiveList<uThread>* ktrq = ktReadyQueue;
	if(!ktrq->empty()){										//If not empty, grab a uThread and run it
		ut = ktrq->front();
		ktrq->pop_front();
	}else{													//If empty try to fill

	    localCluster->tryGetWorks(*ktrq);				//Try to fill the local queue
		if(!ktrq->empty()){									//If there is more work start using it
			ut = ktrq->front();
			ktrq->pop_front();
		}else{												//If no work is available, Switch to defaultUt
			if(kThread::currentKT->currentUT->state == YIELD)	return;				//if the running uThread yielded, continue running it
			ut = mainUT;
		}
	}
	assert(ut != nullptr);
	switchContext(ut,args);
}

void kThread::initialize(bool isDefaultKT) {
	kThread::currentKT		=	this;											//Set the thread_locl pointer to this thread, later we can find the executing thread by referring to this
	kThread::ktReadyQueue = new IntrusiveList<uThread>();

	if(isDefaultKT){
	    mainUT = uThread::create(defaultStackSize);
        mainUT->start(*localCluster, (ptr_t)kThread::defaultRun, this, nullptr, nullptr); //if defaultKT, then create a stack for mainUT cause pthread stack is assigned to initUT
    }
	else{
	    mainUT = uThread::createMainUT(*localCluster);			                    //Default function takes up the default pthread's stack pointer and run from there
	}

	uThread::totalNumberofUTs--; //Default uThreads are not counted as valid work
	mainUT->state =	READY;
	currentUT         =   mainUT;
}

void kThread::defaultRun(void* args) {
    kThread* thisKT = (kThread*) args;
    uThread* ut = nullptr;

    while (true) {
        //TODO: break this loop when total number of uThreads are less than 1, and end the kThread
        thisKT->localCluster->getWork(*thisKT->ktReadyQueue);
        assert(!ktReadyQueue->empty());                         //ktReadyQueue should not be empty at this point
        ut = thisKT->ktReadyQueue->front();
        thisKT->ktReadyQueue->pop_front();

        thisKT->switchContext(ut, nullptr);                     //Switch to the new uThread
    }
}

void kThread::postSwitchFunc(uThread* nextuThread, void* args=nullptr) {

    kThread* ck = kThread::currentKT;
	if(ck->currentUT != kThread::currentKT->mainUT){			//DefaultUThread do not need to be managed here
		switch (ck->currentUT->state) {
			case TERMINATED:
				ck->currentUT->destory(false);
				break;
			case YIELD:
				ck->currentUT->resume();;
				break;
			case MIGRATE:
				ck->currentUT->resume();
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
	ck->currentUT	= nextuThread;								//Change the current thread to the next
	nextuThread->state = RUNNING;
}

std::thread::native_handle_type kThread::getThreadNativeHandle() {
	assert(threadSelf != nullptr);
	return threadSelf->native_handle();
}

std::thread::id kThread::getThreadID() {
	assert(threadSelf != nullptr);
	return threadSelf->get_id();
}
