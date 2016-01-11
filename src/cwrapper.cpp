/*
 * cwrapper.cpp
 *
 *  Created on: Dec 1, 2014
 *      Author: Saman Barghi
 */

#include "cwrapper.h"

#include "generic/basics.h"
#include "runtime/uThread.h"
#include "runtime/uThreadPool.h"
#include "runtime/kThread.h"
#include "io/Network.h"

#ifdef __cplusplus
extern "C"{
#endif
/**************Cluster*************/
WCluster* cluster_create(){  return reinterpret_cast<WCluster*>( new Cluster( ) );}
void cluster_destroy(WCluster* cluster){ delete reinterpret_cast<Cluster*>(cluster); }
WCluster* cluster_get_default(){ return reinterpret_cast<WCluster*>(&Cluster::defaultCluster);}
WCluster* cluster_get_current(){ return reinterpret_cast<WCluster*>((Cluster*)kThread::currentKT->currentUT->getCurrentCluster());}
uint64_t cluster_get_id(WCluster* cluster){ return reinterpret_cast<Cluster*>(cluster)->getClusterID();}
/**********************************/

/*************kThread**************/
WkThread* kThread_create(WCluster* cluster){  return reinterpret_cast<WkThread*>( new kThread( reinterpret_cast<Cluster*>(cluster) ) );}
void WkThread_destroy(WkThread* kt){ delete reinterpret_cast<kThread*>(kt); }
pthread_t kThread_get_current_pthread_id(){ return kThread::currentKT->getThreadNativeHandle(); }
pthread_t kThread_get_pthread_id(WkThread* kt){ return (reinterpret_cast<kThread*>(kt))->getThreadNativeHandle(); }
uint64_t kThread_count(){ return kThread::totalNumberofKTs.load(); }
/**********************************/


/*************uThread**************/
WuThread* uThread_create(void *(*start_routine) (void *), void *arg){ return reinterpret_cast<WuThread*>( uThread::create((funcvoid1_t)start_routine, arg)); }
WuThread* uThread_create_with_cluster(WCluster* cluster, void *(*start_routine) (void *), void *arg){
    return reinterpret_cast<WuThread*>( uThread::create( (funcvoid1_t)start_routine, arg, reinterpret_cast<Cluster*>(cluster) ) );
}
void uThread_migrate(WCluster* cluster){ kThread::currentKT->currentUT->migrate(reinterpret_cast<Cluster*>(cluster)); }
void uThread_destroy(WuThread* ut){ delete reinterpret_cast<uThread*>(ut); }
void uThread_yield(){ kThread::currentKT->currentUT->yield(); }
/**********************************/


/*************Connection**************/
WConnection* connection_create(int fd){ return reinterpret_cast<WConnection*>( new Connection(fd)); }
void connection_destroy(WConnection* con){ delete reinterpret_cast<Connection*>(con); }
ssize_t connection_recvfrom(WConnection* con, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){
    return reinterpret_cast<Connection*>(con)->recvfrom(buf, len, flags, src_addr, addrlen);
}
ssize_t connection_sendmsg(WConnection* con, const struct msghdr *msg, int flags){ return reinterpret_cast<Connection*>(con)->sendmsg(msg, flags);}

ssize_t connection_read(WConnection* con, void *buf, size_t count){ return reinterpret_cast<Connection*>(con)->read(buf, count);}
ssize_t connection_write(WConnection* con, const void *buf, size_t count){ return reinterpret_cast<Connection*>(con)->write(buf, count);}
int connection_close(WConnection* con){ return reinterpret_cast<Connection*>(con)->close();}
void connection_poll_open(WConnection* con){ return reinterpret_cast<Connection*>(con)->pollOpen();}
void connection_poll_reset(WConnection* con){ return reinterpret_cast<Connection*>(con)->pollReset();}
/**********************************/
/******************** uThreadPool **************/
WuThreadPool* uthreadpool_create(){return reinterpret_cast<WuThreadPool*> (new uThreadPool());}
void uthreadpool_destory(WuThreadPool* utp){ delete reinterpret_cast<uThreadPool*>(utp);}
void uthreadpool_execute(WuThreadPool* utp, WCluster* cluster, void *(*start_routine) (void *), void *arg){
    reinterpret_cast<uThreadPool*>(utp)->uThreadExecute((funcvoid1_t)start_routine, arg, reinterpret_cast<Cluster*>(cluster));
}
/**********************************/


#ifdef __cplusplus
}
#endif
