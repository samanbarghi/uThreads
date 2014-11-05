/*
 * uThread.h
 *
 *  Created on: Oct 23, 2014
 *      Author:  Saman Barghi
 */

#ifndef UTHREAD_H_
#define UTHREAD_H_

#include <cstdint>
#include <cstddef>
#include "global.h"


class uThread {
	friend class kThread;
private:

	uThread(funcvoid1_t, void*);							//To create a new uThread, create function should be called
	/*
	 * Thread variables
	 */
	size_t		stackSize;
	priority_t 	priority;				//Threads priority, lower number means higher priority

	/*
	 * Stack Boundary
	 */
	vaddr		stackTop;				//Top of the stack
	vaddr		stackBottom;			//Bottom of the stack

	/*
	 * general functions
	 */

	vaddr createStack(size_t);			//Create a stack with given size

public:

	vaddr 		stackPointer;			// holds stack pointer while thread inactive

	virtual ~uThread();

	uThread(const uThread&) = delete;
	const uThread& operator=(const uThread&) = delete;

	void setPriority(priority_t);
	priority_t getPriority() const;

	/*
	 * Thread management functions
	 */
	static uThread* create(funcvoid1_t, void*);
};

/*
 * An object for comparing uThreads based on their priorities.
 * This will be used in priority queues to determine which uThread
 * should run next.
 */
class CompareuThread{
public:
	bool operator()(uThread* ut1, uThread* ut2){
		int p1 = ut1->getPriority();
		int p2 = ut2->getPriority();
		if(p1 < p2 || p1 == p2){
			return true;
		}else{
			return false;
		}
	}
};

#endif /* UTHREAD_H_ */
