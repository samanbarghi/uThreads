/*
 * ReadyQueue.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "ReadyQueue.h"

using namespace std;
ReadyQueue::ReadyQueue() {
}

ReadyQueue::~ReadyQueue() {
//	delete mtx;
	//mtx = nullptr;
}

uThread ReadyQueue::pop() {
	mtx.lock();
	uThread ut = priorityQueue.top();
	priorityQueue.pop();
	mtx.unlock();
	return ut;
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
