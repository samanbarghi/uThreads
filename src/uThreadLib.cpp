/*
 * uThreadLib.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "uThreadLib.h"
#include <stdio.h>

uThreadLib::uThreadLib() {
	Cluster defalutCluster;				//Create default cluster
	clusters.push_back(defalutCluster);					//Default cluster is always at the front of the vector

	uThread defaultUthread;								//Create the default uThread
	clusters.front().readyQueue.push(defaultUthread);	//Push the default uThread to the default cluster ready queue

}

uThreadLib::~uThreadLib() {
	// TODO Auto-generated destructor stub
}
