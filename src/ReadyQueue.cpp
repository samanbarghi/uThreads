/*
 * ReadyQueue.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "ReadyQueue.h"


ReadyQueue::ReadyQueue() {

}

ReadyQueue::~ReadyQueue() {
	delete priorityQueue;				//Delete the main queue
}

uThread ReadyQueue::pop() {
	return priorityQueue.pop();
}

void ReadyQueue::push(const uThread& ut) {
	priorityQueue.push(ut);
}

size_t ReadyQueue::size() const {
	return priorityQueue.size();
}

bool ReadyQueue::empty() const {
	return priorityQueue.empty();
}
