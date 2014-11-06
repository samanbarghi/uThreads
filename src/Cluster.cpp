/*
 * Cluster.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "Cluster.h"
#include "kThread.h"
#include <iostream>

Cluster	Cluster::defaultCluster;						//Default cluster
std::vector<Cluster*> Cluster::clusters;
uThread* uThread::initUT = new uThread();

Cluster::Cluster() {
	std::cout << "Starting the cluster" << std::endl;
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
	std::cout << "We are going to invoke the thread" << std::endl;
	func(args);
	kThread::currentKT->currentUT->status	= TERMINATED;
	kThread::currentKT->switchContext();
	//Context will be switched in kThread
}

void Cluster::uThreadStart(uThread* ut){
	ut->status	= READY;								//Change status to ready, before pushing it to ready queue just in case context switch occurred before we get to this part
	readyQueue.push(ut);								//Scheduling uThread
}

uThread* Cluster::getWork(){
	return readyQueue.pop();
}

uThread* Cluster::getWorkOrWait() {
	return readyQueue.cvPop();
}
