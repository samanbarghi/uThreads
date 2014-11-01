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
private:

	/*
	 * Thread variables
	 */
	size_t		stackSize;
	priority_t 	priority;				//Threads priority, lower number means higher priority
	ptr_t		func;					//Pointer to the function that is being run by thread

	/*
	 * general functions
	 */

	vaddr createStack(size_t);			//Create a stack with given size

public:

	vaddr 		stackPointer;			// holds stack pointer while thread inactive

	uThread(ptr_t, void*);				//Constructor accepts a function and it's arguments
	virtual ~uThread();

	void setPriority(priority_t);
	priority_t getPriority() const;

	/*
	 * Thread management functions
	 */
	int create(ptr_t, void*);
};

/*
 * An object for comparing uThreads based on their priorities.
 * This will be used in priority queues to determine which uThread
 * should run next.
 */
class CompareuThread{
public:
	bool operator()(uThread& ut1, uThread& ut2){
		int p1 = ut1.getPriority();
		int p2 = ut2.getPriority();
		if(p1 < p2 || p1 == p2){
			return true;
		}else{
			return false;
		}
	}
};

#endif /* UTHREAD_H_ */
