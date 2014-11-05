/*
 * Cluster.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "Cluster.h"
#include "kThread.h"
#include <iostream>
extern thread_local kThread* currentKT;

Cluster	Cluster::defaultCluster;						//Default cluster
std::vector<Cluster*> Cluster::clusters;

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
	std::cout << "We are going to invoke the thread" << std::endl;
	func(args);
	//TODO: terminate uThread
	currentKT->switchContext();
}

void Cluster::uThreadStart(uThread* ut){
	readyQueue.push(ut);														//Scheduling uThread
}

uThread* Cluster::getWork(){
	uThread* ut = readyQueue.pop();
	return ut;
}
