/*
 * Cluster.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "Cluster.h"
#include "kThread.h"
#include <iostream>

std::atomic_ushort Cluster::clusterMasterID(0);

Cluster::Cluster(): numberOfkThreads(0) {
	initialSynchronization();
}

void Cluster::initialSynchronization(){
	clusterID = clusterMasterID++;
}

Cluster::~Cluster() {}

void Cluster::invoke(funcvoid1_t func, void* args) {
	func(args);
	kThread::currentKT->currentUT->status	= TERMINATED;
	kThread::currentKT->switchContext();
	//Context will be switched in kThread
}

void Cluster::uThreadSchedule(uThread* ut) {
	assert(ut != nullptr);
	ut->status	= READY;								//Change status to ready, before pushing it to ready queue just in case context switch occurred before we get to this part
	readyQueue.push(ut);								//Scheduling uThread
}

uThread* Cluster::tryGetWork(){return readyQueue.tryPop();}

//pop more than one uThread from the ready queue and push into the kthread local ready queue
void Cluster::tryGetWorks(IntrusiveList<uThread> &queue){
	readyQueue.tryPopMany(queue, numberOfkThreads.load());
}

void Cluster::getWork(IntrusiveList<uThread> &queue) {
	readyQueue.popMany(queue, numberOfkThreads.load());
}

uint64_t Cluster::getClusterID() const {return clusterID;}
