/*
 * uThreadLib.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "uThreadLib.h"
#include "kThread.h"
#include <stdio.h>
#include <iostream>
#include <unistd.h>

/*
 * Only works for GCC
 */
Cluster* uThreadLib::cluster =  new Cluster();
using namespace std;
uThreadLib::uThreadLib() {
//	int value = 1000;
//	uThread defaultUthread((ptr_t)run, &value );								//Create the default uThread
////	clusters.front().readyQueue.push(defaultUthread);	//Push the default uThread to the default cluster ready queue
//	int value2 = 2000;
//	uThread secondUThread((ptr_t)run, &value2);
//
//	vaddr mainStackPointer;
//	stackSwitch(&mainStackPointer, mainStackPointer, postFunc);
//	kThread kt;
	kThread kt1(cluster);
	uThread* ut;
	int value[1000];
	for (int i=1; i< 2; i++){
		value[i] = i;
		ut = uThread::create((funcvoid1_t)run, &value[i], i);
	}

	while(true){
		uThread::yield();
	}
	sleep(10);
	cout << "Here we go!" << endl;
}

uThreadLib::~uThreadLib() {
	// TODO Auto-generated destructor stub
}

void uThreadLib::run(void* args){
	sleep(1);
	//assume args is an int
	int value = *(int*)args;
	cout << "This is run " <<  value << endl;
	kThread::currentKT->currentUT->migrate(cluster);
	kThread::currentKT->printThreadId();
}
