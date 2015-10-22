/*
 * Network.cpp
 *
 *  Created on: Oct 21, 2015
 *      Author: Saman Barghi
 */
#include "Network.h"
#include "kThread.h"
#include <unistd.h>
#include <sys/types.h>
#include <cassert>

Connection::Connection(int fd, bool poll=false): fd(fd){
   //TODO:throw an exception if fd <= 0
    pd.fd = fd;
    //if user is providing fd,
    //add this to the underlying polling structure
    if(poll)
        kThread::ioHandler->open(this->pd);
};

int Connection::socket(int domain, int type, int protocol){

    int sockfd = ::socket(domain, type, protocol);
    if(sockfd != -1){
        this->fd = sockfd;
        this->pd.fd = sockfd;
    }
    return sockfd;
};

int Connection::listen(int backlog){
    assert(this->fd > 0);
    assert(this->fd == this->pd.fd);

    int res = ::listen(this->fd, backlog);
    //on success add to poll
    if(res == 0)    kThread::ioHandler->open(this->pd);
    return res;
}

int Connection::accept(Connection *conn, struct sockaddr *addr, socklen_t *addrlen){
    assert(this->fd != -1);
    //check connection queue for wating connetions
    //Set the fd as nonblocking
    int sockfd = ::accept4(this->fd, addr, addrlen, SOCK_NONBLOCK );
    while( (sockfd == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        //User level blocking using nonblocking io
        kThread::ioHandler->block(this->pd, UT_IOREAD);
        sockfd = ::accept4(this->fd, addr, addrlen, 0);
    }
    //otherwise return the result
    if(sockfd > 0){
        conn->setFD(sockfd);
        kThread::ioHandler->open(conn->pd);
    }
    return sockfd;
}

ssize_t Connection::recv(void *buf, size_t len, int flags){
    assert(buf != nullptr);
    assert(this->fd != -1);

    ssize_t res = ::recv(this->fd, (void*)buf, len, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
           //User level blocking using nonblocking io
           kThread::ioHandler->block(this->pd, UT_IOREAD);
           res = ::recv(this->fd, buf, len, flags);
    }
    return res;
}
ssize_t Connection::send(const void *buf, size_t len, int flags){
    assert(buf != nullptr);
    assert(this->fd != -1);

    ssize_t res = ::send(this->fd, buf, len, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        kThread::ioHandler->block(this->pd, UT_IOWRITE);
        res = ::send(this->fd, buf, len, flags);
    }
    return res;
}

int Connection::close(){
    this->pd.closing.store(true);
    kThread::ioHandler->close(this->pd);
    return ::close(this->fd);
}

Connection::~Connection(){

}
