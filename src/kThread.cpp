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

thread_local kThread* currentKT;												//Current Running kThread

/*
 * Create a new kThread who belongs to the defaultCluster
 */

kThread::kThread() : threadSelf(&kThread::run, this, nullptr), localCluster(&Cluster::defaultCluster){
}


kThread::kThread(Cluster* cluster) : threadSelf(&kThread::run, this, cluster), localCluster(cluster) {
}


kThread::~kThread() {
	threadSelf.join();															//wait for the thread to terminate properly
}

void kThread::run(Cluster* cl) {
	this->initialize(cl);														//This is necessary cause run is being called before other constructor initialization
	std::cout << "Started Running! ID: " << std::this_thread::get_id() << std::endl;
	switchContext(defaultUt);
}

void kThread::switchContext(uThread* ut) {
	//TODO: should take care of the currentThread, push it back to readyqueue
	stackSwitch(&currentUT->stackPointer, ut->stackPointer, uThreadLib::postFunc);
}

void kThread::switchContext(){
	uThread* ut = localCluster->getWork();
	if(ut == nullptr){ut = defaultUt;}											//If no work is available, Switch to defaultUt

	switchContext(ut);
}

void kThread::initialize(Cluster* cl) {
	currentKT		=	this;													//Set the thread_locl pointer to this thread, later we can find the executing thread by referring to this

	defaultUt = new uThread((funcvoid1_t)kThread::defaultRun, this);			//Default function should not be put back on readyQueue
	currentUT		= 	defaultUt;

	if(cl == nullptr){localCluster = &Cluster::defaultCluster;}
	else{localCluster = cl;}
}

void kThread::defaultRun(void* args) {
	kThread* thisKT = (kThread*) args;
	while(true){
		std::cout << "Sleep on ReadyQueue" << std::endl;
		uThread* ut = thisKT->localCluster->readyQueue.cvPop();
		thisKT->switchContext(ut);
	}
}

//TODO: Get rid of this. For test purposes only
void kThread::printThreadId(){

	std::thread::id main_thread_id = std::this_thread::get_id();

	std::cout << "This is my thread id: " << main_thread_id << std::endl;
}

