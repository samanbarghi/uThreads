/*
 * Pthread.cpp
 *
 *  Created on: Dec 1, 2014
 *      Author: Saman Barghi
 */

#include "global.h"
#include "uThread.h"
#include "kThread.h"

/* extern "C" int uthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg){

	//TODO: Where to delete these? Just wait till the program ends?
	Cluster* cluster = new Cluster();
	kThread* kt = new kThread(cluster);
	uThread* ut = uThread::create((funcvoid1_t)start_routine, arg, cluster);

	*thread = kt->getThreadNativeHandle();
	return 0;
}

extern "C" int uthread_join(pthread_t thread, void **value_ptr){
	//do nothing since we are doing this in kThread
	return 0;
}


extern "C" void* uthread_migrate_to_syscall(){
	const Cluster* currentCluster = kThread::currentKT->currentUT->getCurrentCluster();
	kThread::currentKT->currentUT->migrate(&Cluster::syscallCluster);
	return (void*)currentCluster;
}
extern "C" void uthread_migrate_back_from_syscall(void* cc){
	Cluster* currentCluster = (Cluster*)cc;
	kThread::currentKT->currentUT->migrate(currentCluster);
}
extern "C" void uthread_create_syscall_kthread(cpu_set_t cset){
	kThread* kt = new kThread(&Cluster::syscallCluster);

	if(pthread_setaffinity_np((pthread_t)kt->getThreadNativeHandle(), sizeof(cpu_set_t), &cset) < 0){
		perror("Could not set affinity\n");
	}
} */
