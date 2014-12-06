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
Cluster	Cluster::defaultCluster;						//Default cluster, ID: 1
Cluster	Cluster::syscallCluster;						//syscall cluster, ID: 2
uThread* uThread::initUT = new uThread();

Cluster::Cluster() {
	clusters.push_back(this);
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
	ut->status	= READY;								//Change status to ready, before pushing it to ready queue just in case context switch occurred before we get to this part
	readyQueue.push(ut);								//Scheduling uThread
}

uThread* Cluster::tryGetWork(){
	return readyQueue.pop();
}

uThread* Cluster::getWork() {
	return readyQueue.cvPop();
}

