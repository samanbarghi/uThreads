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
#include "uThreadCache.h"
#include <iostream>		//TODO: remove this, add debug object
#include <cassert>

//TODO: change all pointers to unique_ptr or shared_ptr
/* initialize all static members here */
std::atomic_ulong uThread::totalNumberofUTs(0);
std::atomic_ulong uThread::uThreadMasterID(0);

uThreadCache uThread::utCache;
Cluster Cluster::defaultCluster;
uThread* uThread::initUT = nullptr;
kThread kThread::defaultKT(true);

/******************************************/
void uThread::reset(){
   stackPointer = (vaddr)this;                //reset stack pointer
   currentCluster = nullptr;
   state=INITIALIZED;
}

void uThread::destory(bool force=false) {
    //check whether we should cache it or not
    totalNumberofUTs--;
    if(force || (utCache.push(this)<0)){
        free((ptr_t)(stackBottom));                              //Free the allocated memory for stack
    }
}

vaddr uThread::createStack(size_t ssize) {
    ptr_t st = malloc(ssize);
	if(st == nullptr)
		exit(-1);										//TODO: Proper exception
	return ((vaddr)st);
}

uThread* uThread::create(size_t ss){
    uThread* ut = utCache.pop();

    if(ut == nullptr){
        vaddr mem = uThread::createStack(ss);               //Allocating stack for the thread
        vaddr This = mem + ss - sizeof(uThread);
        ut = new (ptr_t(This)) uThread(mem, ss);
    }else{
        totalNumberofUTs++;
    }
    return ut;

}

uThread* uThread::createMainUT(Cluster& cluster){
   uThread* ut = new uThread(0,0);
   ut->currentCluster = &cluster;
   return ut;
}

void uThread::start(const Cluster& cluster, ptr_t func, ptr_t args1, ptr_t args2, ptr_t args3) {
    currentCluster = const_cast<Cluster*>(&cluster);
    stackPointer = (vaddr)stackInit(stackPointer, (ptr_t)Cluster::invoke, (ptr_t) func, args1, args2, args3);			//Initialize the thread context
    assert(stackPointer != 0);
    this->resume();
}

void uThread::start(ptr_t func, ptr_t arg1, ptr_t arg2, ptr_t arg3){
    return start(Cluster::defaultCluster, func, arg1, arg2, arg3);
}

void uThread::yield(){
    kThread* ck = kThread::currentKT;
    assert(ck != nullptr);
    assert(ck->currentUT != nullptr);
	ck->currentUT->state = YIELD;
	ck->switchContext();
}

void uThread::migrate(Cluster* cluster){
	assert(cluster != nullptr);
	if(kThread::currentKT->localCluster == cluster)     //no need to migrate
		return;
	kThread::currentKT->currentUT->currentCluster= cluster;
	kThread::currentKT->currentUT->state = MIGRATE;
	kThread::currentKT->switchContext();
}

void uThread::suspend(std::function<void()>& func) {
	state = WAITING;
	kThread::currentKT->switchContext(&func);
}

void uThread::resume(){
    if(state== WAITING || state== INITIALIZED || state == MIGRATE || state == YIELD){
        state = READY;
        currentCluster->schedule(this);          //Put thread back to readyqueue
    }
}

void uThread::terminate(){
	kThread::currentKT->currentUT->state = TERMINATED;
	kThread::currentKT->switchContext();                //It's scheduler job to switch to another context and terminate this thread
}

uThread* uThread::currentUThread () const   { return kThread::currentKT->currentUT; }
