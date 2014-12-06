/*
 * Pthread.h
 *
 *  Created on: Nov 26, 2014
 *      Author: Saman Barghi
 */

#ifndef PTHREAD_H_
#define PTHREAD_H_
#include <pthread.h>

extern int uthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
extern int uthread_join(pthread_t thread, void **value_ptr);
extern void* uthread_migrate_to_syscall();
extern void uthread_migrate_back_from_syscall();
extern void uthread_create_syscall_kthread(cpu_set_t);					//Create a kThread in syscall cluster with specified affinity


#endif /* PTHREAD_H_ */
