/*
 * ReadyQueue.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "ReadyQueue.h"
#include "kThread.h"
#include <iostream>

ReadyQueue::ReadyQueue() {
}

ReadyQueue::~ReadyQueue() {
}

uThread* ReadyQueue::pop() {
	if(queue.front() == queue.fence())
		return nullptr;
	std::unique_lock<std::mutex> mlock(mtx);
	uThread* ut = queue.front();
	queue.pop_front();
	mlock.unlock();
	return ut;
}

uThread* ReadyQueue::cvPop() {
	std::unique_lock<std::mutex> mlock(mtx);
//	std::cout << "cvPop:" << kThread::currentKT->localCluster->clusterID << std::endl;
	while(queue.front() == queue.fence()){
		cv.wait(mlock);
	}
	uThread* ut = queue.front();
	queue.pop_front();
	mlock.unlock();
	return ut;
}

void ReadyQueue::push(uThread* ut) {
	std::unique_lock<std::mutex> mlock(mtx);
	queue.push_back(ut);
	mlock.unlock();
	cv.notify_one();
}


bool ReadyQueue::empty() const {
	return queue.empty();
}
