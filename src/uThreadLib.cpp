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
//  Cluster* cluster = &clusters.front();
	kThread kt;
	int value = 100000;
	uThread* ut = uThread::create((funcvoid1_t)run, &value);
//	kThread kt1;

	cout << "Here we go!" << endl;
}

uThreadLib::~uThreadLib() {
	// TODO Auto-generated destructor stub
}

void uThreadLib::run(void* args){
	//assume args is an int
	int value = *(int*)args;
	cout << "This is run " <<  value << endl;
}

void uThreadLib::postFunc(vaddr* current, vaddr next) {
	cout << "This is the after function! : " << endl;
}
