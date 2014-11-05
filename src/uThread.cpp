/*
 * uThread.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "uThread.h"
#include "Cluster.h"
#include "kThread.h"
#include <iostream>		//TODO: remove this, add debug object
extern thread_local kThread* currentKT;			//current kt

uThread::uThread(funcvoid1_t func, void* args) {
	/*
	 * IMPORTANT: ALL INSTANCES OF UTHREAD SHOULD BE CREATED THROUGH CREATE
	 * CAUSE STACK SHOULD BE ALLOCATED ON HEAP
	 */
	priority 	= default_uthread_priority;				//If no priority is set, set to the default priority
	stackSize	= default_stack_size;					//Set the stack size to default
	stackPointer= createStack(stackSize);				//Allocating stack for the thread

	stackPointer = stackInit(stackPointer, (ptr_t)Cluster::invoke, func, args, nullptr, nullptr);			//Initialize the thread context
}

uThread::~uThread() {
	free(stackPointer);									//Free the allocated memory for stack
	//This should never been called directly!
}

vaddr uThread::createStack(size_t ssize) {
	stackTop = malloc(ssize);
	if(stackTop == nullptr)
		exit(-1);										//TODO: Proper exception
	stackBottom = (char*) stackTop + ssize;
	return (vaddr)stackBottom;
}

priority_t uThread::getPriority() const {
	return priority;
}

void uThread::setPriority(priority_t priority) {
	this->priority = priority;
}

uThread* uThread::create(funcvoid1_t func, void* args) {
	//TODO: add a function that accepts priority value
	uThread* ut = new uThread(func, args);
	//TODO:	This should go to the currentKT, figure out the problem
	Cluster::defaultCluster.uThreadStart(ut);			//schedule current ut
	return ut;
}
