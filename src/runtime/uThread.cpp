/*
 * uThread.cpp
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#include "uThread.h"
#include "Cluster.h"
#include "kThread.h"
#include "BlockingSync.h"
#include <iostream>		//TODO: remove this, add debug object
#include <cassert>

//TODO: change all pointers to unique_ptr or shared_ptr
/* initialize all static members here */
std::atomic_ulong uThread::totalNumberofUTs(0);
std::atomic_ulong uThread::uThreadMasterID(0);


Cluster Cluster::defaultCluster;
kThread kThread::defaultKT(true);
uThread uThread::initUT(Cluster::defaultCluster);

Cluster Cluster::ioCluster;
kThread kThread::ioKT(&Cluster::ioCluster);
uThread uThread::ioUT(Cluster::ioCluster, IOHandler::defaultIOFunc, nullptr, nullptr, nullptr);

/******************************************/

/*
 * This will only be called by the default uThread.
 * Default uThread does not have a stack and rely only on
 * The current running pthread's stack
 */
uThread::uThread(Cluster& cluster){
	stackSize	= 0;                                    //Stack size depends on kernel thread's stack
	stackPointer= 0;                                    //We don't know where on stack we are yet
	status 		= RUNNING;
	currentCluster = const_cast<Cluster*>(&cluster);
	initialSynchronization();
}
uThread::uThread(const Cluster& cluster, funcvoid1_t func, ptr_t args1, ptr_t args2, ptr_t args3){

	stackSize	= default_stack_size;                   //Set the stack size to default
	stackPointer= createStack(stackSize);               //Allocating stack for the thread
	status		= INITIALIZED;
	currentCluster = const_cast<Cluster*>(&cluster);
	initialSynchronization();
	stackPointer = (vaddr)stackInit(stackPointer, (funcvoid1_t)Cluster::invoke, (ptr_t) func, args1, args2, args3);			//Initialize the thread context
	assert(stackPointer != 0);
}

uThread::~uThread() {
	free((ptr_t)stackTop);                              //Free the allocated memory for stack
	//This should never be called directly! terminate should be called instead
}
void uThread::decrementTotalNumberofUTs() {
	totalNumberofUTs--;
}

void uThread::initialSynchronization() {
	totalNumberofUTs++;
	uThreadID = uThreadMasterID++;
}

vaddr uThread::createStack(size_t ssize) {
    ptr_t st = malloc(ssize);
	if(st == nullptr)
		exit(-1);										//TODO: Proper exception
	stackTop = (vaddr)st;
	stackBottom =  stackTop + ssize;
	return stackBottom;
}

uThread* uThread::create(const Cluster& cluster, funcvoid1_t func, ptr_t args1, ptr_t args2, ptr_t args3) {
	uThread* ut = new uThread(cluster, func, args1, args2, args3);
	/*
	 * if it is the main thread it goes to the the defaultCluster,
	 * Otherwise to the related cluster
	 */
	ut->currentCluster->uThreadSchedule(ut);            //schedule current ut
	return ut;
}

uThread* uThread::create(funcvoid1_t func, ptr_t arg1, ptr_t arg2, ptr_t arg3){
    return create(Cluster::defaultCluster, func, arg1, arg2, arg3);
}

void uThread::yield(){
    kThread* ck = kThread::currentKT;
    assert(ck != nullptr);
    assert(ck->currentUT != nullptr);
	ck->currentUT->status = YIELD;
	ck->switchContext();
}

void uThread::migrate(Cluster* cluster){
	assert(cluster != nullptr);
	if(kThread::currentKT->localCluster == cluster)     //no need to migrate
		return;
	kThread::currentKT->currentUT->currentCluster= cluster;
	kThread::currentKT->currentUT->status = MIGRATE;
	kThread::currentKT->switchContext();
}

void uThread::suspend(std::function<void()>& func) {
	status = WAITING;
	kThread::currentKT->switchContext(&func);
}

void uThread::resume(){
    if(status == WAITING)
        currentCluster->uThreadSchedule(this);          //Put thread back to readyqueue
}

void uThread::terminate(){
	//TODO: This should take care of locks as well ?
	decrementTotalNumberofUTs();
	delete this;										//Suicide
}

void uThread::uexit(){
	kThread::currentKT->currentUT->status = TERMINATED;
	kThread::currentKT->switchContext();                //It's scheduler job to switch to another context and terminate this thread
}
/*
 * Setters and Getters
 */
const Cluster& uThread::getCurrentCluster() const {return *currentCluster;}
uint64_t uThread::getTotalNumberofUTs() {return totalNumberofUTs;}
uint64_t uThread::getUthreadId() const { return uThreadID;}


