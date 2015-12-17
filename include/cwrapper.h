/*
 * cwrapper.h
 *
 *  Created on: Nov 26, 2014
 *      Author: Saman Barghi
 */

#ifndef CWRAPPER_H_
#define CWRAPPER_H_
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>


#ifdef __cplusplus
extern "C"{
#endif

/**************Cluster*************/
struct WCluster;
typedef struct WCluster WCluster;
WCluster* cluster_create();
void cluster_destruct(WCluster* cluster);
WCluster* cluster_get_default();//return default Cluster
WCluster* cluster_get_current();
uint64_t cluster_get_id(WCluster* cluster);
/**********************************/

/*************kThread**************/
struct WkThread;
typedef struct WkThread WkThread;
WkThread* kThread_create(WCluster* cluster);
void kThread_destroy(WkThread* kt);
//if linux
pthread_t kThread_get_current_pthread_id(); //return pthread_t for current running thread
pthread_t kThread_get_pthread_id(WkThread* kt); //return pthread_t for the provided kThread
//endif linux
uint64_t kThread_count();
/**********************************/

/*************uThread**************/
struct WuThread;
typedef struct WuThread WuThread;
WuThread* uThread_create(void *(*start_routine) (void *), void *arg);
WuThread* uThread_create_with_cluster(WCluster* cluster, void *(*start_routine) (void *), void *arg);
void uThread_migrate(WCluster* cluster);
void uThread_destroy(WuThread* ut);
void uThread_yield();
/**********************************/

/*************Connection************/
struct WConnection;
typedef struct WConnection WConnection;
WConnection* connection_create(int fd);
void connection_destory(WConnection* c);

ssize_t connection_recvfrom(WConnection* con, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t connection_sendmsg(WConnection* con, const struct msghdr *msg, int flags);

ssize_t connection_read(WConnection* con, void *buf, size_t count);
ssize_t connection_write(WConnection* con, const void *buf, size_t count);

int connection_close(WConnection* con);
void connection_poll_open(WConnection* con);
void connection_poll_reset(WConnection* con);
/**********************************/

/******************** uThreadPool **************/
struct WuThreadPool;
typedef struct WuThreadPool WuThreadPool;
WuThreadPool* uthreadpool_create();
void uthreadpool_destory(WuThreadPool* utp);
void uthreadpool_execute(WuThreadPool* utp, WCluster* cluster, void *(*start_routine) (void *), void *arg);
/**********************************/
#ifdef __cplusplus
}
#endif

#endif /* CWRAPPER_H_ */
