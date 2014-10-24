/*
 * uThread.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "uThread.h"
uThread::uThread() {
	priority = DEFAULT_UTHREAD_PRIORITY;				//If no priority is set, se to the default priority
}

uThread::~uThread() {
	// TODO Auto-generated destructor stub
}

priority_t uThread::getPriority() const {
	return priority;
}

void uThread::setPriority(priority_t priority) {
	this->priority = priority;
}
