/*
 * kThread.cpp
 *
 *  Created on: Oct 27, 2014
 *      Author: Saman Barghi
 */

#include "kThread.h"
#include "uThreadLib.h" //TODO: remove this, only for testing purposes
#include <iostream>
#include <unistd.h>
__thread kThread* kThread::currentKT = nullptr;
kThread* kThread::defaultKT = new kThread(true);

kThread::kThread(bool initial) : localCluster(&Cluster::defaultCluster){		//This is only for initial kThread
	//threadSelf does not need to be initialized
	this->initialize();
	currentUT = uThread::initUT;												//Current running uThread is the inital one
}

kThread::kThread(Cluster* cluster) : localCluster(cluster) {
	threadSelf = new std::thread(&kThread::run, this);
}

kThread::kThread() : localCluster(&Cluster::defaultCluster){
	threadSelf = new std::thread(&kThread::run, this);
}

kThread::~kThread() {
	threadSelf->join();															//wait for the thread to terminate properly
}

void kThread::run() {
	this->initialize();
	std::cout << "Started Running! ID: " << std::this_thread::get_id() << std::endl;
	switchContext(mainUT);
}

void kThread::switchContext(uThread* ut) {
	//TODO: should take care of the currentThread, push it back to readyqueue
	stackSwitch(ut, &currentUT->stackPointer, ut->stackPointer, postSwitchFunc);
}

void kThread::switchContext(){
	uThread* ut = localCluster->getWork();
	if(ut == nullptr){ut = mainUT;}												//If no work is available, Switch to defaultUt
	else{std::cout << "Got WORK" << std::endl;}
	//TODO: if yeild, it should continue with the current thread !
	switchContext(ut);
}

void kThread::initialize() {
	kThread::currentKT		=	this;											//Set the thread_locl pointer to this thread, later we can find the executing thread by referring to this

	this->mainUT = new uThread((funcvoid1_t)kThread::defaultRun, this, default_uthread_priority);			//Default function should not be put back on readyQueue, or be scheduled by cluster
	uThread::totalNumberofUTs--;												//Default uThreads are not counted as valid work
	this->mainUT->status	=	READY;
	this->currentUT		= 	this->mainUT;
}

void kThread::defaultRun(void* args) {
	kThread* thisKT = (kThread*) args;
	while(true){
		std::cout << "Sleep on ReadyQueue" << std::endl;
		uThread* ut = thisKT->localCluster->getWorkOrWait();
		thisKT->switchContext(ut);
	}
}

void kThread::postSwitchFunc(uThread* nextuThread) {
	std::cout << "This is post func" << std:: endl;

	if(kThread::currentKT->currentUT != kThread::currentKT->mainUT){			//DefaultUThread do not need to be managed here
		switch (kThread::currentKT->currentUT->status) {
			case TERMINATED:
				std::cout << "TERMINATED" << std::endl;
				kThread::currentKT->currentUT->terminate();
				break;
			case YIELD:
				kThread::currentKT->localCluster->readyQueue.push(kThread::currentKT->currentUT);
				break;
			//TODO: if status is waiting or blocked or ..., should be put back on the appropriate ready queue
			default:
				break;
		}
	}
	kThread::currentKT->currentUT	= nextuThread;								//Change the current thread to the next
	nextuThread->status = RUNNING;
}

//TODO: Get rid of this. For test purposes only
void kThread::printThreadId(){
	std::thread::id main_thread_id = std::this_thread::get_id();
	std::cout << "This is my thread id: " << main_thread_id << std::endl;
}

