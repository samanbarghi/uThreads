/*
 * kThread.h
 *
 *  Created on: Oct 27, 2014
 *      Author: Saman Barghi
 */

#ifndef KTHREAD_H_
#define KTHREAD_H_
#include <thread>
#include "Cluster.h"
#include "uThread.h"
#include "global.h"


class kThread {
	friend class uThread;
private:
	std::thread threadSelf;					//pthread related to the current thread
	uThread* defaultUt;						//Each kThread has a default uThread that is used when there is no work available

	uThread* currentUT;						//Pointer to the current running ut
	Cluster* localCluster;					//Pointer to the cluster that provides jobs for this kThread

	void run(Cluster*);						//The run function for the thread.
	void initialize(Cluster*);				//Initialization function for kThread
public:
	kThread();
	kThread(Cluster*);
	virtual ~kThread();


	void switchContext(uThread*);			//Put current uThread in ready Queue and run the passed uThread
	void switchContext();					//Put current uThread in readyQueue and pop a new uThread to run
	static void defaultRun(void*) __noreturn;

	void printThreadId();
};

#endif /* KTHREAD_H_ */
