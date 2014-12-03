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
	friend class Cluster;
	friend class ReadyQueue;
private:
	kThread(bool);							//This is only for the initial kThread
	std::thread *threadSelf;				//pthread related to the current thread
	uThread* mainUT;						//Each kThread has a default uThread that is used when there is no work available

	static kThread* defaultKT;				//default main thread of the application
	static kThread* syscallKT;				//syscall kernel thread for the application


	Cluster* localCluster;					//Pointer to the cluster that provides jobs for this kThread

	void run();						//The run function for the thread.
	void initialize();				//Initialization function for kThread
	static void postSwitchFunc(uThread*, void*) __noreturn;

public:
	kThread();
	kThread(Cluster*);
	virtual ~kThread();

	uThread* currentUT;						//Pointer to the current running ut
	static __thread kThread* currentKT;

	static mword totalNumberofKTs;

	void switchContext(uThread*,void* args = nullptr);			//Put current uThread in ready Queue and run the passed uThread
	void switchContext(void* args = nullptr);					//Put current uThread in readyQueue and pop a new uThread to run
	static void defaultRun(void*) __noreturn;


	void printThreadId();
	std::thread::native_handle_type getThreadNativeHandle();
};

#endif /* KTHREAD_H_ */
