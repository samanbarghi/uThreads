/*
 * Cluster.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "Cluster.h"
#include "kThread.h"
#include "ReadyQueue.h"
#include "io/IOHandler.h"

std::atomic_ushort Cluster::clusterMasterID(0);

Cluster::Cluster(): numberOfkThreads(0) {
    //TODO: IO handler should be only applicable for IO Clusters
    //or be created with the first IO call
    readyQueue = new ReadyQueue();
	initialSynchronization();
    iohandler  = IOHandler::create(*this);
}

void Cluster::initialSynchronization(){
	clusterID = clusterMasterID++;
}

Cluster::~Cluster() {}

void Cluster::invoke(funcvoid3_t func, ptr_t arg1, ptr_t arg2, ptr_t arg3) {
	func(arg1, arg2, arg3);
	kThread::currentKT->currentUT->state	= TERMINATED;
	kThread::currentKT->switchContext();
	//Context will be switched in kThread
}

void Cluster::schedule(uThread* ut) {
	assert(ut != nullptr);
	readyQueue->push(ut);								//Scheduling uThread
}

void Cluster::scheduleMany(IntrusiveList<uThread>& queue, size_t count){
    assert(!queue.empty());
    readyQueue->pushMany(queue, count);
}

uThread* Cluster::tryGetWork(){return readyQueue->tryPop();}

//pop more than one uThread from the ready queue and push into the kthread local ready queue
ssize_t Cluster::tryGetWorks(IntrusiveList<uThread> &queue){
	return readyQueue->tryPopMany(queue);
}

ssize_t Cluster::getWork(IntrusiveList<uThread> &queue) {
	return readyQueue->popMany(queue);
}
