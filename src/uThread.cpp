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
uint64_t uThread::totalNumberofUTs = 0;
uint64_t uThread::uThreadMasterID= 0;
std::mutex uThread::uThreadSyncLock;

/*
 * This will only be called by the default uThread.
 * Default uThread does not have a stack and rely only on
 * The current running pthread's stack
 */
uThread::uThread(Cluster* cluster){
	priority 	= default_uthread_priority;				//If no priority is set, set to the default priority
	stackSize	= 0;									//Stack size depends on kernel thread's stack
	stackPointer= nullptr;								//We don't know where on stack we are yet
	status 		= RUNNING;
	currentCluster = cluster;
	initialSynchronization();
//	std::cout << "Default uThread" << std::endl;
}

uThread::uThread(funcvoid1_t func, ptr_t args, priority_t pr, Cluster* cluster = nullptr) : priority(pr) {

	stackSize	= default_stack_size;					//Set the stack size to default
	stackPointer= createStack(stackSize);				//Allocating stack for the thread
	status		= INITIALIZED;
	if(cluster) this->currentCluster = cluster;
	else this->currentCluster = kThread::currentKT->localCluster;
	initialSynchronization();
	stackPointer = (vaddr)stackInit(stackPointer, (funcvoid1_t)Cluster::invoke, (ptr_t) func, args, nullptr, nullptr);			//Initialize the thread context
}

uThread::~uThread() {
	free(stackTop);									//Free the allocated memory for stack
	//This should never be called directly! terminate should be called instead
}
void uThread::decrementTotalNumberofUTs() {
	std::lock_guard<std::mutex> lock(uThreadSyncLock);
	totalNumberofUTs--;
}

void uThread::initialSynchronization() {
	std::lock_guard<std::mutex> lock(uThreadSyncLock);
	totalNumberofUTs++;
	this->uThreadID = uThreadMasterID++;
}

vaddr uThread::createStack(size_t ssize) {
	stackTop = malloc(ssize);
	if(stackTop == nullptr)
		exit(-1);										//TODO: Proper exception
	stackBottom = (char*) stackTop + ssize;
	return (vaddr)stackBottom;
}

//TODO: a create function that accepts a Cluster
uThread* uThread::create(funcvoid1_t func, void* args) {
	uThread* ut = new uThread(func, args, default_uthread_priority);
	/*
	 * if it is the main thread it goes to the the defaultCluster,
	 * Otherwise to the related cluster
	 */
	ut->currentCluster->uThreadSchedule(ut);			//schedule current ut
	return ut;
}

uThread* uThread::create(funcvoid1_t func, void* args, priority_t pr) {
	uThread* ut = new uThread(func, args, pr);
	/*
	 * if it is the main thread it goes to the the defaultCluster,
	 * Otherwise to the related cluster
	 */
	ut->currentCluster->uThreadSchedule(ut);			//schedule current ut
	return ut;
}

uThread* uThread::create(funcvoid1_t func, void* args, Cluster* cluster) {
	uThread* ut = new uThread(func, args, default_uthread_priority, cluster);
//	std::cerr << "CREATED: " << ut->id << " ADRESS:" << ut  << " STACKBOTTOM: " << ut->stackBottom  << " Pointer: " << ut->stackPointer << std::endl;
	/*
	 * if it is the main thread it goes to the the defaultCluster,
	 * Otherwise to the related cluster
	 */
	ut->currentCluster->uThreadSchedule(ut);			//schedule current ut
	return ut;
}

void uThread::yield(){
//	std::cout << "YEIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIILD" << std::endl;
    kThread* ck = kThread::currentKT;
	ck->currentUT->status = YIELD;
	ck->switchContext();
}

void uThread::migrate(Cluster* cluster){
	assert(cluster != nullptr);
	if(kThread::currentKT->localCluster == cluster)				//no need to migrate
		return;
//	std::cout << "MIGRAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAATE" << std::endl;
	kThread::currentKT->currentUT->currentCluster= cluster;
	kThread::currentKT->currentUT->status = MIGRATE;
//	std::cout << "Migrating to Cluster:" << cluster->clusterID << std::endl;
	kThread::currentKT->switchContext();
}

void uThread::suspend(BlockingQueue* bqueue, std::mutex& lock) {
	this->status = WAITING;
	QueueAndLock* qal = new QueueAndLock();
	qal->list = &bqueue->queue;
	qal->mutex= &lock;
	qal->umutex= nullptr;

	kThread::currentKT->switchContext(qal);
}
void uThread::suspend(BlockingQueue* bqueue, Mutex& mutex) {
	this->status = WAITING;
	QueueAndLock* qal = new QueueAndLock();
	qal->list = &bqueue->queue;
	qal->mutex= nullptr;
	qal->umutex= &mutex;

	kThread::currentKT->switchContext(qal);
}
void uThread::suspend(IOHandler* ioh){
    this->status = IOBLOCK;
    kThread::currentKT->switchContext();
}

void uThread::resume(){
    if(this->status == WAITING || this->status == IOBLOCK)
        this->currentCluster->uThreadSchedule(this);		//Put thread back to readyqueue
}

void uThread::terminate(){
	//TODO: This should take care of locks as well ?
	decrementTotalNumberofUTs();
	delete this;										//Suicide
}

void uThread::uexit(){
	kThread::currentKT->currentUT->status = TERMINATED;
	kThread::currentKT->switchContext();				//It's scheduler job to switch to another context and terminate this thread
}
/*
 * Setters and Getters
 */
priority_t uThread::getPriority() const {return priority;}
void uThread::setPriority(priority_t priority) {this->priority = priority;}
const Cluster* uThread::getCurrentCluster() const {return this->currentCluster;}
uint64_t uThread::getTotalNumberofUTs() {return totalNumberofUTs;}
uint64_t uThread::getUthreadId() const { return this->uThreadID;}


