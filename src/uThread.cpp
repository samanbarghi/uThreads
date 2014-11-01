/*
 * uThread.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "uThread.h"
#include <iostream>

using namespace std;

uThread::uThread(ptr_t func,  void* args) {

	this->create(func, args);
}

uThread::~uThread() {
	// TODO Auto-generated destructor stub
}

vaddr uThread::createStack(size_t ssize) {
	vaddr stack = malloc(ssize);
	if(stack == nullptr)
		exit(-1);

	cout << "Stack just got created: " << stack << endl;
	printf("%p\n", stack);
	return stack;
}

priority_t uThread::getPriority() const {
	return priority;
}

void uThread::setPriority(priority_t priority) {
	this->priority = priority;
}

int uThread::create(ptr_t func, void* args) {
	//TODO: add a function that accepts priority value
	priority 	= default_uthread_priority;				//If no priority is set, set to the default priority
	stackSize	= default_stack_size;					//Set the stack size to default
	stackPointer= createStack(stackSize);				//Create a stack with the default stack size


	stackInit(stackPointer, func, args, nullptr, nullptr, nullptr);			//Initialize the thread context
}
