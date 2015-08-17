/*
 * Cluster.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "Cluster.h"
#include "kThread.h"
#include <iostream>

std::vector<Cluster*> Cluster::clusters;

std::mutex Cluster::clusterSyncLock;
uint64_t Cluster::clusterMasterID = 0;

Cluster::Cluster(): numberOfkThreads(0) {

	clusters.push_back(this);
	initialSynchronization();
}

void Cluster::initialSynchronization(){
	std::lock_guard<std::mutex> lock(clusterSyncLock);
	clusterID = clusterMasterID++;
}

Cluster::~Cluster() {
	/*TODO: Make sure to call this at the end of the program !!!!
	 * for(Cluster* c:clusters){
		free(c);
	}
	clusters.erase(clusters.front(), clusters.end());*/
}

void Cluster::invoke(funcvoid1_t func, void* args) {
//	std::cout << "We are going to invoke the thread" << std::endl;
	func(args);
	kThread::currentKT->currentUT->status	= TERMINATED;
//	std::cout << "After run function: " << kThread::currentKT->currentUT << std::endl;
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
void Cluster::tryGetWorks(EmbeddedList<uThread> *queue){
	assert(queue != nullptr);
	readyQueue.tryPopMany(queue, numberOfkThreads);
}

void Cluster::getWork(EmbeddedList<uThread> *queue) {
	assert(queue != nullptr);
	readyQueue.popMany(queue, numberOfkThreads);
}

uint64_t Cluster::getClusterID() const {return this->clusterID;}
