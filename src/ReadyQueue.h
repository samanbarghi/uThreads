/*
 * ReadyQueue.h
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#ifndef READYQUEUE_H_
#define READYQUEUE_H_
#include <queue>
#include <mutex>
#include <condition_variable>
#include "uThread.h"


class ReadyQueue {
private:
	std::priority_queue<uThread, std::vector<uThread>, CompareuThread> priorityQueue;
public:
	ReadyQueue();
	virtual ~ReadyQueue();

	uThread pop();
	void push(const uThread&);
	size_t size() const;
	bool empty() const;
};

#endif /* READYQUEUE_H_ */
