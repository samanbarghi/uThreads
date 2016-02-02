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
WCluster* cluster_create(){  return reinterpret_cast<WCluster*>( new Cluster( ) );}
void cluster_destroy(WCluster* cluster){ delete reinterpret_cast<Cluster*>(cluster); }
WCluster* cluster_get_default(){ return reinterpret_cast<WCluster*>(&Cluster::getDefaultCluster());}
WCluster* cluster_get_current(){ return reinterpret_cast<WCluster*>(const_cast<Cluster*>(&uThread::currentUThread()->getCurrentCluster()));}
uint64_t cluster_get_id(WCluster* cluster){ return reinterpret_cast<Cluster*>(cluster)->getClusterID();}
/**********************************/

/*************kThread**************/
WkThread* kThread_create(WCluster* cluster){  return reinterpret_cast<WkThread*>( new kThread( *reinterpret_cast<Cluster*>(cluster) ) );}
void WkThread_destroy(WkThread* kt){ delete reinterpret_cast<kThread*>(kt); }
pthread_t kThread_get_current_pthread_id(){ return kThread::currentkThread()->getThreadNativeHandle(); }
pthread_t kThread_get_pthread_id(WkThread* kt){ return (reinterpret_cast<kThread*>(kt))->getThreadNativeHandle(); }
uint64_t kThread_count(){ return kThread::getTotalNumberOfkThreads(); }
/**********************************/


/*************uThread**************/
WuThread* uThread_create(WCluster* cluster, void *(*start_routine) (void *), void *arg){
    uThread* ut = uThread::create();
    ut->start(*reinterpret_cast<Cluster*>(cluster), (ptr_t)start_routine, arg );
    return reinterpret_cast<WuThread*>( ut);
}
void uThread_migrate(WCluster* cluster){ uThread::currentUThread()->migrate(reinterpret_cast<Cluster*>(cluster)); }
void uThread_terminate(WuThread* ut){  reinterpret_cast<uThread*>(ut)->terminate(); }
void uThread_yield(){ uThread::currentUThread()->yield(); }
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
