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

#include "generic/basics.h"
#include "cwrapper.h"
#include "runtime/uThread.h"
#include "runtime/uThreadPool.h"
#include "runtime/kThread.h"
#include "io/Network.h"

#ifdef __cplusplus
extern "C"{
#endif
/**************Cluster*************/
WCluster*   cluster_create(){  return reinterpret_cast<WCluster*>( new Cluster( ) );}
void        cluster_destroy(WCluster* cluster){ delete reinterpret_cast<Cluster*>(cluster); }
WCluster*   cluster_get_default(){ return reinterpret_cast<WCluster*>(&Cluster::getDefaultCluster());}
WCluster*   cluster_get_current(){ return reinterpret_cast<WCluster*>(const_cast<Cluster*>(&uThread::currentUThread()->getCurrentCluster()));}
uint64_t    cluster_get_id(WCluster* cluster){ return reinterpret_cast<Cluster*>(cluster)->getID();}
size_t      cluster_get_number_of_kThreads(WCluster* cluster){ return reinterpret_cast<Cluster*>(cluster)->getNumberOfkThreads();}
/**********************************/

/*************kThread**************/
WkThread* kThread_create(WCluster* cluster){  return reinterpret_cast<WkThread*>( new kThread( *reinterpret_cast<Cluster*>(cluster) ) );}
void WkThread_destroy(WkThread* kt){ delete reinterpret_cast<kThread*>(kt); }
WkThread* kThread_get_current(){ return reinterpret_cast<WkThread*>(kThread::currentkThread());}
//if linux
pthread_t kThread_get_current_pthread_id(){ return kThread::currentkThread()->getThreadNativeHandle(); }
pthread_t kThread_get_pthread_id(WkThread* kt){ return (reinterpret_cast<kThread*>(kt))->getThreadNativeHandle(); }
//endif
uint64_t kThread_count(){ return kThread::getTotalNumberOfkThreads(); }
/**********************************/


/*************uThread**************/
WuThread*   uThread_create(bool joinable){ return reinterpret_cast<WuThread*>(uThread::create(joinable));}
void        uThread_start(WuThread* ut, WCluster* cluster, void* func, void *arg1, void* arg2, void* arg3){
    reinterpret_cast<uThread*>(ut)->start(*reinterpret_cast<Cluster*>(cluster), (ptr_t)func, arg1, arg2, arg3 );
}
void        uThread_migrate(WCluster* cluster){ uThread::currentUThread()->migrate(reinterpret_cast<Cluster*>(cluster)); }
void        uThread_terminate(WuThread* ut){  reinterpret_cast<uThread*>(ut)->terminate(); }
void        uThread_yield(){ uThread::currentUThread()->yield(); }
bool        uThread_join(WuThread* ut){ return reinterpret_cast<uThread*>(ut)->join();}
void        uThread_detach(WuThread* ut){  reinterpret_cast<uThread*>(ut)->detach();};
uint64_t    uThread_get_id(WuThread* ut){ return reinterpret_cast<uThread*>(ut)->getID(); }
WuThread*   uThread_get_current(){ return reinterpret_cast<WuThread*>(uThread::currentUThread());}
uint64_t    uThread_get_total_number_of_uThreads(){ return uThread::getTotalNumberofUTs();}
/**********************************/


/*************Connection**************/
WConnection* connection_create(){return reinterpret_cast<WConnection*>( new Connection()); }
WConnection* connection_create_with_fd(int fd){ return reinterpret_cast<WConnection*>( new Connection(fd)); }
WConnection* connection_create_socket(int domain, int type, int protocol){ return reinterpret_cast<WConnection*>(new Connection(domain, type, protocol));}
void         connection_destroy(WConnection* con){ delete reinterpret_cast<Connection*>(con); }

int          connection_accept(WConnection* acceptor, WConnection *conn, struct sockaddr *addr, socklen_t *addrlen){
    return reinterpret_cast<Connection*>(acceptor)->accept(reinterpret_cast<Connection*>(conn), addr, addrlen);
}
WConnection* connection_accept_connenction(WConnection* acceptor, struct sockaddr *addr, socklen_t *addrlen){
    return reinterpret_cast<WConnection*>(reinterpret_cast<Connection*>(acceptor)->accept(addr, addrlen));
}

int          connection_socket(WConnection* conn, int domain, int type, int protocol){
    return reinterpret_cast<Connection*>(conn)->socket(domain, type, protocol);
}

int          connection_listen(WConnection* conn, int backlog){return reinterpret_cast<Connection*>(conn)->listen(backlog);}
int          connection_bind(WConnection* conn, const struct sockaddr *addr,socklen_t addrlen){return reinterpret_cast<Connection*>(conn)->bind(addr, addrlen);}
int          connection_connect(WConnection* conn, const struct sockaddr *addr,socklen_t addrlen){return reinterpret_cast<Connection*>(conn)->connect(addr, addrlen);}
int          connection_close(WConnection* conn){ return reinterpret_cast<Connection*>(conn)->close();}

ssize_t     connection_recv(WConnection* conn, void *buf, size_t len, int flags){
    reinterpret_cast<Connection*>(conn)->recv(buf, len, flags);
}
ssize_t     connection_recvfrom(WConnection* conn, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){
    reinterpret_cast<Connection*>(conn)->recvfrom(buf, len, flags, src_addr, addrlen);
};
ssize_t     connection_recvmsg(WConnection* conn, int sockfd, struct msghdr *msg, int flags){
    reinterpret_cast<Connection*>(conn)->recvmsg(sockfd, msg, flags);
};
int         connection_recvmmsg(WConnection* conn, int sockfd, struct mmsghdr *msgvec, unsigned int vlen, unsigned int flags, struct timespec *timeout){
    reinterpret_cast<Connection*>(conn)->recvmmsg(sockfd, msgvec, vlen, flags, timeout);
};


ssize_t     connection_send(WConnection* conn, const void *buf, size_t len, int flags){
    return reinterpret_cast<Connection*>(conn)->send(buf, len, flags);
}
ssize_t     connection_sendto(WConnection* conn, int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen){
    return reinterpret_cast<Connection*>(conn)->sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}
ssize_t      connection_sendmsg(WConnection* conn, const struct msghdr *msg, int flags){
    return reinterpret_cast<Connection*>(conn)->sendmsg(msg, flags);
}
int         connection_sendmmsg(WConnection* conn, int sockfd, struct mmsghdr *msgvec, unsigned int vlen, unsigned int flags){
    return reinterpret_cast<Connection*>(conn)->sendmmsg(sockfd, msgvec, vlen, flags);
}

ssize_t connection_read(WConnection* conn, void *buf, size_t count){ return reinterpret_cast<Connection*>(conn)->read(buf, count);}
ssize_t connection_write(WConnection* conn, const void *buf, size_t count){ return reinterpret_cast<Connection*>(conn)->write(buf, count);}

void connection_block_on_read(WConnection* conn){ reinterpret_cast<Connection*>(conn)->blockOnRead();}
void connection_block_on_write(WConnection* conn){ reinterpret_cast<Connection*>(conn)->blockOnWrite();}

int         connection_get_fd(WConnection* conn){return reinterpret_cast<Connection*>(conn)->getFd();}

/**********************************/

/******************** Mutex *********************/
struct      WMutex;
typedef     struct WMutex WMutex;
WMutex*     mutex_create(){return reinterpret_cast<WMutex*>(new Mutex());}
void        mutex_destroy(WMutex* mutex){ delete reinterpret_cast<Mutex*>(mutex);}
bool        mutex_acquire(WMutex* mutex){ return reinterpret_cast<Mutex*>(mutex)->acquire();}
void        mutex_release(WMutex* mutex){ reinterpret_cast<Mutex*>(mutex)->release();}
/**********************************/

/******************** OwnlerLock ****************/
WOwnerLock* ownerlock_create(){return reinterpret_cast<WOwnerLock*>(new OwnerLock());}
void        ownerlock_destroy(WOwnerLock* olock){ delete reinterpret_cast<OwnerLock*>(olock);}
uint64_t       ownerlock_acquire(WOwnerLock* olock){ return reinterpret_cast<OwnerLock*>(olock)->acquire();}
void        ownerlock_release(WOwnerLock* olock){ reinterpret_cast<OwnerLock*>(olock)->release();}
/**********************************/

/******************** ConditionVariable *********/
WConditionVariable* condition_variable_create(){ return reinterpret_cast<WConditionVariable*>(new ConditionVariable());}
void                condition_variable_destroy(WConditionVariable* cv){ delete reinterpret_cast<ConditionVariable*>(cv);}
void                condition_variable_wait(WConditionVariable* cv, WMutex* mutex){reinterpret_cast<ConditionVariable*>(cv)->wait(*reinterpret_cast<Mutex*>(mutex));}
void                condition_variable_signal(WConditionVariable* cv, WMutex* mutex){reinterpret_cast<ConditionVariable*>(cv)->signal(*reinterpret_cast<Mutex*>(mutex));}
void                condition_variable_signall_all(WConditionVariable* cv, WMutex* mutex){reinterpret_cast<ConditionVariable*>(cv)->signalAll(*reinterpret_cast<Mutex*>(mutex));}
bool                condition_variable_empty(WConditionVariable* cv){reinterpret_cast<ConditionVariable*>(cv)->empty();}
/**********************************/

/******************** Semaphore ****************/
WSemaphore* semaphore_create(){return reinterpret_cast<WSemaphore*>(new Semaphore());}
void        semaphore_destroy(WSemaphore* sem){ delete reinterpret_cast<Semaphore*>(sem);}
bool        semaphore_p(WSemaphore* sem){ return reinterpret_cast<Semaphore*>(sem)->P();}
void        semaphore_v(WSemaphore* sem){ reinterpret_cast<Semaphore*>(sem)->V();}
/**********************************/

/******************** uThreadPool **************/
WuThreadPool* uthreadpool_create(){return reinterpret_cast<WuThreadPool*> (new uThreadPool());}
void uthreadpool_destory(WuThreadPool* utp){ delete reinterpret_cast<uThreadPool*>(utp);}
void uthreadpool_execute(WuThreadPool* utp, WCluster* cluster, void *(*start_routine) (void *), void *arg){
    reinterpret_cast<uThreadPool*>(utp)->uThreadExecute((funcvoid1_t)start_routine, arg, *reinterpret_cast<Cluster*>(cluster));
}
/**********************************/


#ifdef __cplusplus
}
#endif
