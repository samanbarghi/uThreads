/*
 * uThread.h
 *
 *  Created on: Oct 23, 2014
 *      Author:  Saman Barghi
 */
#pragma once
#include <mutex>
#include <atomic>
#include "generic/basics.h"
#include "generic/IntrusiveContainers.h"
#include "Stack.h"

class BlockingQueue;
class Mutex;
class IOHandler;
class Cluster;
class uThreadCache;

//Thread states
enum uThreadState {
    INITIALIZED,                                                    //uThread is initialized
    READY,                                                          //uThread is in a ReadyQueue
    RUNNING,                                                        //uThread is Running
    YIELD,                                                          //uThread Yields
    MIGRATE,                                                        //Migrate to another cluster
    WAITING,                                                        //uThread is in waiting mode
    IOBLOCK,                                                        //uThread is blocked on IO
    TERMINATED                                                      //uThread is done and should be terminated
};


class uThread : public IntrusiveList<uThread>::Link{
    friend class uThreadCache;
	friend class kThread;
	friend class Cluster;
	friend class LibInitializer;
	friend class BlockingQueue;
	friend class IOHandler;
private:

	//TODO: Add a function to check uThread's stack for overflow ! Prevent overflow or throw an exception or error?
	//TODO: Add a debug object to project, or a dtrace or lttng functionality
	//TODO: Check all functions and add assertions wherever it is necessary


	uThread(vaddr sb, size_t ss) :
	stackPointer(vaddr(this)), stackBottom(sb), stackSize(ss),
	state(INITIALIZED), uThreadID(uThreadMasterID++), currentCluster(nullptr){
	    totalNumberofUTs++;
	}

	static uThread* createMainUT(Cluster&);      //Only to create init and main uThreads

	static uThread*	initUT;             //initial uT that is associated with main

	static vaddr createStack(size_t);   //Create a stack with given size

	/*
	 * Statistics variables
	 */
	//TODO: Add more variables, number of suspended, number of running ...
	static std::atomic_ulong totalNumberofUTs;			//Total number of existing uThreads
	static std::atomic_ulong uThreadMasterID;			//The main ID counter
	uint64_t uThreadID;							//unique Id for this uthread

	/*
	 * Thread variables
	 */
	uThreadState state;				//Current state of the uThread, should be private only friend classes can change this
	Cluster*	currentCluster;			//This will be used for migrating to a new Cluster

	/*
	 * Stack Boundary
	 */
	size_t		stackSize;
	vaddr 		stackPointer;			// holds stack pointer while thread inactive
	vaddr		stackBottom;			//Bottom of the stack

	/*
	 * general functions
	 */
	void destory(bool);                     //destroy the stack which destroys the uThread
	void reset();                       //reset stack pointers
	void suspend(std::function<void()>&);
public:

	uThread(const uThread&) = delete;
	const uThread& operator=(const uThread&) = delete;

	const Cluster& getCurrentCluster() const;
	/*
	 * Thread management functions
	 */
    static uThread* create(size_t ss);
    static uThread* create(){ return create(defaultStackSize);}

	void start(const Cluster& cluster, ptr_t func, ptr_t arg1=nullptr, ptr_t arg2=nullptr, ptr_t arg3=nullptr);
	void start(ptr_t func, ptr_t arg1=nullptr, ptr_t arg2=nullptr, ptr_t arg3=nullptr);

	static void yield();
	static void terminate();
	void migrate(Cluster*);				//Migrate the thread to a new Cluster
	void resume();

	/*
	 * general functions
	 */
	static uint64_t getTotalNumberofUTs();
	uint64_t getUthreadId() const;
};
