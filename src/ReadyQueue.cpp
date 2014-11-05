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
}

uThread* ReadyQueue::pop() {
	std::lock_guard<std::mutex> lock(mtx);
	if(priorityQueue.empty())
		return nullptr;
	uThread* ut = priorityQueue.top();
	priorityQueue.pop();
	return ut;
}

uThread* ReadyQueue::cvPop() {
	std::unique_lock<std::mutex> mlock(mtx);
	while(priorityQueue.empty()){
		cv.wait(mlock);
	}
	uThread* ut = priorityQueue.top();
	priorityQueue.pop();
	return ut;
}

void ReadyQueue::push(uThread* ut) {
	std::unique_lock<std::mutex> mlock(mtx);
	priorityQueue.push(ut);
	mlock.unlock();
	cv.notify_one();
}

size_t ReadyQueue::size() const {
	return priorityQueue.size();
}

bool ReadyQueue::empty() const {
	return priorityQueue.empty();
}
