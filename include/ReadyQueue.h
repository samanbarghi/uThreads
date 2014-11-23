/*
 * ReadyQueue.h
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#ifndef READYQUEUE_H_
#define READYQUEUE_H_
#include <mutex>
#include <condition_variable>
#include "uThread.h"

class ReadyQueue {
private:
//	std::priority_queue<uThread*, std::vector<uThread*>, CompareuThread> priorityQueue;
	EmbeddedList<uThread> queue;
	std::mutex mtx;
	std::condition_variable cv;
public:
	ReadyQueue();
	virtual ~ReadyQueue();

	uThread* pop();
	uThread* cvPop();				//Pop with condition variable
	void push(uThread*);
	bool empty() const;
};

#endif /* READYQUEUE_H_ */
