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

class Cluster;

class uThread {
	friend class kThread;
	friend class Cluster;
private:

	uThread();										//This will be called by default uThread
	uThread(funcvoid1_t, ptr_t, priority_t);		//To create a new uThread, create function should be called

	static uThread*	initUT;				//initial uT that is associated with main

	/*
	 * Statistics variables
	 */
	static uint64_t totalNumberofUTs;	//Total number of existing uThreads
	/*
	 * Thread variables
	 */
	size_t		stackSize;
	priority_t 	priority;				//Threads priority, lower number means higher priority
	uThreadStatus status;				//Current status of the uThread, should be private only friend classes can change this
	Cluster*	destinationCluster;		//This will be used for migrating to a new Cluster

	/*
	 * Stack Boundary
	 */
	vaddr 		stackPointer;			// holds stack pointer while thread inactive
	vaddr		stackTop;				//Top of the stack
	vaddr		stackBottom;			//Bottom of the stack

	/*
	 * general functions
	 */

	vaddr createStack(size_t);			//Create a stack with given size

public:

	virtual ~uThread();

	uThread(const uThread&) = delete;
	const uThread& operator=(const uThread&) = delete;

	void setPriority(priority_t);
	priority_t getPriority() const;

	/*
	 * Thread management functions
	 */
	static uThread* create(funcvoid1_t, void*);
	static uThread* create(funcvoid1_t, void*, priority_t);

	static void yield();
	void migrate(Cluster*);		//Migrate the thread to a new Cluster
	void terminate();

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
