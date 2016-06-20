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
#include <inttypes.h>

/**
 * @file cwrapper.h
 * @author Saman Barghi
 * @brief C Wrapper for uThreads.
 */

#ifdef __cplusplus
extern "C"{
#endif

/**************Cluster*************/
/** @name Cluster
 *  C interface for class Cluster.
 *  @{
 */
struct      WCluster;
typedef     struct WCluster WCluster;
/** @copydoc Cluster::Cluster */
WCluster*   cluster_create();
/** @copydoc Cluster::~Cluster */
void        cluster_destroy(WCluster* cluster);
/** @copydoc Cluster::getDefaultCluster */
WCluster*   cluster_get_default();//return default Cluster
/** @copydoc uThread::getCurrent*/
WCluster*   cluster_get_current();
/** @copydoc Cluster::getID */
uint64_t    cluster_get_id(WCluster* cluster);
/** @copydoc Cluster::getNumberOfkThreads */
size_t      cluster_get_number_of_kThreads(WCluster* cluster);
/** @} **/
/**********************************/

/*************kThread**************/
struct WkThread;
typedef struct WkThread WkThread;
WkThread* kThread_create(WCluster* cluster);
void kThread_destroy(WkThread* kt);
WkThread* kThread_get_current();
//if linux
/** @copydoc kThread::getThreadNativeHandle */
pthread_t   kThread_get_current_pthread_id(); //return pthread_t for current running thread
/** @copydoc kThread::getThreadNativeHandle */
pthread_t   kThread_get_pthread_id(WkThread* kt); //return pthread_t for the provided kThread
//endif linux
/** @} */
/**********************************/

/*************uThread**************/
/** @name uThread
 *  C interface for class uThread.
 *  @{
 */
struct      WuThread;
typedef     struct WuThread WuThread;
/** @copydoc uThread::create **/
WuThread*   uThread_create(bool joinable);
/** @copydoc uThread::start **/
void        uThread_start(WuThread* ut, WCluster* cluster, void *func, void *arg1 , void* arg2, void* arg3);
/** @copydoc uThread::migrate **/
void        uThread_migrate(WCluster* cluster);
/** @copydoc uThread::terminate **/
void        uThread_terminate(WuThread* ut);
/** @copydoc uThread::yield **/
void        uThread_yield();
/** @copydoc uThread::join **/
bool        uThread_join(WuThread* ut);
/** @copydoc uThread::detach **/
void        uThread_detach(WuThread* ut);
/** @copydoc uThread::getID **/
uint64_t    uThread_get_id(WuThread* ut);
/** @copydoc uThread::currentUThread**/
WuThread*   uThread_get_current();
/** @copydoc uThread::getTotalNumberofUTs **/
uint64_t uThread_get_total_number_of_uThreads();
///@}
/**********************************/

/*************Connection************/
/** @name Connection
 *  C interface for class Connection.
 *  @{
 */
struct       WConnection;
typedef      struct WConnection WConnection;
/// @copydoc Connection::Connection()
WConnection* connection_create();
/// @copydoc Connection::Connection(int fd)
WConnection* connection_create_with_fd(int fd);
/// @copydoc Connection::Connection(int domain, int type, int protocol)
WConnection* connection_create_socket(int domain, int type, int protocol);
/// @copydoc Connection::~Connection()
void         connection_destory(WConnection* c);


/// @copydoc Connection::accept(Connection *conn, struct sockaddr *addr, socklen_t *addrlen)
int          connection_accept(WConnection* acceptor, WConnection *conn, struct sockaddr *addr, socklen_t *addrlen);
/// @copydoc Connection::accept(struct sockaddr *addr, socklen_t *addrlen)
WConnection* connection_accept_connenction(WConnection* acceptor, struct sockaddr *addr, socklen_t *addrlen);


/// @copydoc Connection::socket
int          connection_socket(WConnection* conn, int domain, int type, int protocol);

/// @copydoc Connection::listen
int          connection_listen(WConnection* conn, int backlog);
/// @copydoc Connection::bind
int          connection_bind(WConnection* conn, const struct sockaddr *addr,socklen_t addrlen);
/// @copydoc Connection::connect
int          connection_connect(WConnection* conn, const struct sockaddr *addr,socklen_t addrlen);
/// @copydoc Connection::close
int          connection_close(WConnection* conn);

/// @copydoc Connection::recv
ssize_t     connection_recv(WConnection* conn, void *buf, size_t len, int flags);
/// @copydoc Connection::recv
ssize_t     connection_recvfrom(WConnection* conn, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
/// @copydoc Connection::recv
ssize_t     connection_recvmsg(WConnection* conn, int sockfd, struct msghdr *msg, int flags);
/// @copydoc Connection::recv
int         connection_recvmmsg(WConnection* conn, int sockfd, struct mmsghdr *msgvec, unsigned int vlen, unsigned int flags, struct timespec *timeout);

/// @copydoc Connection::recv
ssize_t     connection_send(WConnection* conn, const void *buf, size_t len, int flags);
/// @copydoc Connection::recv
ssize_t     connection_sendto(WConnection* conn, int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
/// @copydoc Connection::recv
ssize_t     connection_sendmsg(WConnection* conn, const struct msghdr *msg, int flags);
/// @copydoc Connection::recv
int         connection_sendmmsg(WConnection* conn, int sockfd, struct mmsghdr *msgvec, unsigned int vlen, unsigned int flags);

/// @copydoc Connection::recv
ssize_t     connection_read(WConnection* conn, void *buf, size_t count);
/// @copydoc Connection::recv
ssize_t     connection_write(WConnection* conn, const void *buf, size_t count);


/** @copydoc Connection::blockOnRead */
void        connection_block_on_read(WConnection* conn);

/** @copydoc Connection::blockOnWrite **/
void        connection_block_on_write(WConnection* conn);

/// @copydoc Connection::getFD
int         connection_get_fd(WConnection* conn);
///@}
/**********************************/


/******************** Mutex *********************/
/** @name Mutex
 *  C interface for class Mutex.
 *  @{
 */
struct      WMutex;
typedef     struct WMutex WMutex;
/** @copydoc Mutex::Mutex */
WMutex*     mutex_create();
/** @copydoc Mutex::~Mutex */
void        mutex_destroy(WMutex* mutex);
/** @copydoc Mutex::acquire */
bool        mutex_acquire(WMutex* mutex);
/** @copydoc Mutex::release */
void        mutex_release(WMutex* mutex);
///@}
/**********************************/

/******************** OwnlerLock ****************/
/** @name OwnerLock
 *  C interface for class OwnerLock.
 *  @{
 */
struct      WOwnerLock;
typedef     struct WOwnerLock WOwnerLock;
/** @copydoc OwnlerLock::OwnlerLock */
WOwnerLock* ownerlock_create();
/** @copydoc OwnlerLock::~OwnlerLock */
void        ownerlock_destroy(WOwnerLock* olock);
/** @copydoc OwnlerLock::acquire */
uint64_t        ownerlock_acquire(WOwnerLock* olock);
/** @copydoc OwnlerLock::release */
void        ownerlock_release(WOwnerLock* olock);
///@}
/**********************************/

/******************** ConditionVariable *********/
/** @name ConditionVariable
 *  C interface for class ConditionVariable.
 *  @{
 */
struct      WConditionVariable;
typedef     struct WConditionVariable WConditionVariable;
/** @copydoc ConditionVariable::ConditionVariable */
WConditionVariable* condition_variable_create();
/** @copydoc ConditionVariable::~ConditionVariable */
void                condition_variable_destroy(WConditionVariable* cv);
/** @copydoc ConditionVariable::wait */
void                condition_variable_wait(WConditionVariable* cv, WMutex* mutex);
/** @copydoc ConditionVariable::signal */
void                condition_variable_signal(WConditionVariable* cv, WMutex* mutex);
/** @copydoc ConditionVariable::signalAll */
void                condition_variable_signall_all(WConditionVariable* cv, WMutex* mutex);
/** @copydoc ConditionVariable::empty */
bool                condition_variable_empty(WConditionVariable* cv);
///@}
/**********************************/

/******************** Semaphore ****************/
/** @name Semaphore
 *  C interface for class Semaphore.
 *  @{
 */
struct      WSemaphore;
typedef     struct WSemaphore WSemaphore;
/** @copydoc Semaphore::Semaphore */
WSemaphore* semaphore_create();
/** @copydoc Semaphore::~Semaphore */
void        semaphore_destroy(WSemaphore* sem);
/** @copydoc Semaphore::P */
bool        semaphore_p(WSemaphore* sem);
/** @copydoc Semaphore::V */
void        semaphore_v(WSemaphore* sem);
///@}
/**********************************/

/******************** uThreadPool **************/
/** @name uThreadPool
 *  C interface for class uThreadPool.
 *  @{
 */
struct WuThreadPool;
typedef struct WuThreadPool WuThreadPool;
WuThreadPool* uthreadpool_create();
void uthreadpool_destory(WuThreadPool* utp);
void uthreadpool_execute(WuThreadPool* utp, WCluster* cluster, void *(*start_routine) (void *), void *arg);
///@}
/**********************************/
#ifdef __cplusplus
}
#endif

#endif /* UTHREADS_CWRAPPER_H_ */
