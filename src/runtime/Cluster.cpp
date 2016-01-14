/*
 * Cluster.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "Cluster.h"
#include "kThread.h"
#include "ReadyQueue.h"
#include <iostream>

std::atomic_ushort Cluster::clusterMasterID(0);

Cluster::Cluster(): numberOfkThreads(0) {
    readyQueue = new ReadyQueue();
	initialSynchronization();
}

void Cluster::initialSynchronization(){
	clusterID = clusterMasterID++;
}

Cluster::~Cluster() {}

void Cluster::invoke(funcvoid1_t func, void* args) {
	func(args);
	kThread::currentKT->currentUT->state	= TERMINATED;
	kThread::currentKT->switchContext();
	//Context will be switched in kThread
}

void Cluster::schedule(uThread* ut) {
	assert(ut != nullptr);
	readyQueue->push(ut);								//Scheduling uThread
}

uThread* Cluster::tryGetWork(){return readyQueue->tryPop();}

//pop more than one uThread from the ready queue and push into the kthread local ready queue
ssize_t Cluster::tryGetWorks(IntrusiveList<uThread> &queue){
	return readyQueue->tryPopMany(queue);
}

ssize_t Cluster::getWork(IntrusiveList<uThread> &queue) {
	return readyQueue->popMany(queue);
}
