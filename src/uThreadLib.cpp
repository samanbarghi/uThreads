/*
 * uThreadLib.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "uThreadLib.h"
#include <stdio.h>
#include <iostream>
using namespace std;
uThreadLib::uThreadLib() {
	clusters.emplace_back();							//Default cluster is always at the front of the vector

	int value = 1000;
	uThread defaultUthread((ptr_t)run, &value );								//Create the default uThread
//	clusters.front().readyQueue.push(defaultUthread);	//Push the default uThread to the default cluster ready queue
	int value2 = 2000;
	uThread secondUThread((ptr_t)run, &value2);

	vaddr currentStackPointer;
	stackSwitch(&currentStackPointer, defaultUthread.stackPointer, postFunc);

	cout << "Here we go!" << endl;
}

uThreadLib::~uThreadLib() {
	// TODO Auto-generated destructor stub
}

void uThreadLib::run(void* args){
	//assume args is an int
	int* val = (int*)args;
	cout<<"Value of int is" << *val << endl;
}

void uThreadLib::postFunc(vaddr* current, vaddr next) {
	cout << "This is the after function! : " << endl;
	cout << next << endl;
	cout << current << endl;
	void* p = NULL;
	printf("%p", (void*)&p);
	fflush(stdout);
	cout << "goh" << endl;
}
