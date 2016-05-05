/*******************************************************************************
 *     Copyright © 2015, 2016 Saman Barghi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

#ifndef UTHREADS_CWRAPPER_H_
#define UTHREADS_CWRAPPER_H_

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
WkThread* kThread_get_current();
//if linux
pthread_t kThread_get_current_pthread_id(); //return pthread_t for current running thread
pthread_t kThread_get_pthread_id(WkThread* kt); //return pthread_t for the provided kThread
//endif linux
uint64_t kThread_count();
/**********************************/

/*************uThread**************/
struct WuThread;
typedef struct WuThread WuThread;
WuThread* uThread_create(WCluster* cluster, void *(*start_routine) (void *), void *arg);
void uThread_migrate(WCluster* cluster);
void uThread_terminate(WuThread* ut);
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

void connection_block_on_read(WConnection* conn);
void connection_block_on_read(WConnection* conn);

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

#endif /* UTHREADS_CWRAPPER_H_ */
