/*
 * kThread.h
 *
 *  Created on: Oct 27, 2014
 *      Author: Saman Barghi
 */

#ifndef KTHREAD_H_
#define KTHREAD_H_
#include <thread>

class kThread {
private:
	std::thread threadSelf;


	void run();								//The run function for the thread.
public:
	kThread();
	virtual ~kThread();
};

#endif /* KTHREAD_H_ */
