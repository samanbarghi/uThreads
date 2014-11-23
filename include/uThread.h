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
#include <mutex>
#include "global.h"
#include "EmbeddedList.h"

class BlockingQueue;
class Mutex;
class Cluster;

class QueueAndLock {
	friend class uThread;
	friend class kThread;
	EmbeddedList<uThread>* list;
	std::mutex* mutex;
	Mutex* umutex;
};
class uThread : public EmbeddedList<uThread>::Element{
	friend class kThread;
	friend class Cluster;
private:

	//TODO: Add a function to check uThread's stack for overflow ! Prevent overflow or throw an exception or error?
	//TODO: Fix uThread.h includes: if this is the file that is being included to use the library, it should include at least kThread and Cluster headers
	//TODO: Check for memory leaks, fix terminate function
	//TODO: Add a debug object to project
	//TODO: Check all functions and add assertions wherever it is necessary

	uThread();							//This will be called by default uThread
	uThread(funcvoid1_t, ptr_t, priority_t, Cluster* cluster = nullptr);		//To create a new uThread, create function should be called

	static uThread*	initUT;				//initial uT that is associated with main

	/*
	 * Statistics variables
	 */
	//TODO: Add more variables, number of suspended, number of running ...
	/*
	 * Thread variables
	 */
	size_t		stackSize;
	priority_t 	priority;				//Threads priority, lower number means higher priority
	uThreadStatus status;				//Current status of the uThread, should be private only friend classes can change this
	Cluster*	currentCluster;			//This will be used for migrating to a new Cluster

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
	void terminate();

public:

	static uint64_t totalNumberofUTs;	//Total number of existing uThreads
	virtual ~uThread();

	uThread(const uThread&) = delete;
	const uThread& operator=(const uThread&) = delete;

	void setPriority(priority_t);
	priority_t getPriority() const;
	const Cluster* getCurrentCluster() const;

	/*
	 * Thread management functions
	 */
	static uThread* create(funcvoid1_t, void*);
	static uThread* create(funcvoid1_t, void*, priority_t);
	static uThread* create(funcvoid1_t, void*, Cluster*);

	static void yield();
	void migrate(Cluster*);		//Migrate the thread to a new Cluster
	void suspend(BlockingQueue*,std::mutex&);
	void suspend(BlockingQueue*,Mutex&);
	void resume();
	static void uexit();				//End the thread

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
