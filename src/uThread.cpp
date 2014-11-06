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
uint64_t uThread::totalNumberofUTs = 0;
/*
 * This will only be called by the default uThread.
 * Default uThread does not have a stack and rely only on
 * The current running pthread's stack
 */
uThread::uThread(){
	priority 	= default_uthread_priority;				//If no priority is set, set to the default priority
	stackSize	= 0;									//Stack size depends on kernel thread's stack
	stackPointer= nullptr;								//We don't know where on stack we are yet
	status 		= RUNNING;
	totalNumberofUTs++;

	//defaultStackInit(&this->stackPointer);
	std::cout << "Default uThread" << std::endl;
}
uThread::uThread(funcvoid1_t func, ptr_t args, priority_t pr) : priority(pr) {

	stackSize	= default_stack_size;					//Set the stack size to default
	stackPointer= createStack(stackSize);				//Allocating stack for the thread
	status		= INITIALIZED;
	totalNumberofUTs++;

	stackPointer = stackInit(stackPointer, (ptr_t)Cluster::invoke, func, args, nullptr, nullptr);			//Initialize the thread context
}

uThread::~uThread() {
	free(stackPointer);									//Free the allocated memory for stack
	//This should never be called directly! terminate should be called instead
}

vaddr uThread::createStack(size_t ssize) {
	stackTop = malloc(ssize);
	if(stackTop == nullptr)
		exit(-1);										//TODO: Proper exception
	stackBottom = (char*) stackTop + ssize;
	return (vaddr)stackBottom;
}

//TODO: a create function that accepts a Cluster
uThread* uThread::create(funcvoid1_t func, void* args) {
	uThread* ut = new uThread(func, args, default_uthread_priority);
	/*
	 * if it is the main thread it goes to the the defaultCluster,
	 * Otherwise to the related cluster
	 */
	kThread::currentKT->localCluster->uThreadStart(ut);			//schedule current ut
	return ut;
}

uThread* uThread::create(funcvoid1_t func, void* args, priority_t pr) {
	uThread* ut = new uThread(func, args, pr);
	/*
	 * if it is the main thread it goes to the the defaultCluster,
	 * Otherwise to the related cluster
	 */
	kThread::currentKT->localCluster->uThreadStart(ut);			//schedule current ut
	return ut;
}

void uThread::yield(){
	std::cout << "YEIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIILD" << std::endl;
	kThread::currentKT->currentUT->status = YIELD;
	kThread::currentKT->switchContext();
}

void uThread::terminate(){
	totalNumberofUTs--;
}
/*
 * Setters and Getters
 */
priority_t uThread::getPriority() const {
	return priority;
}

void uThread::setPriority(priority_t priority) {
	this->priority = priority;
}
