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

#include "Network.h"
#include "IOHandler.h"
#include <unistd.h>
#include <sys/types.h>
#include <cassert>
#include <iostream>
Connection::Connection(int domain, int type, int protocol)
                        throw(std::system_error){

    //Make sure to create a nonblocking socket for the connection
    // kernels > 2.6.27
    int sockfd = ::socket(domain, type | SOCK_NONBLOCK, protocol);
    if(sockfd != -1){
        ioh = uThread::currentUThread()->getCurrentCluster().iohandler;
        fd = sockfd;
        pd.fd = sockfd;
    }else{
        throw std::system_error(EFAULT, std::system_category());
    }
}

void Connection::init(int fd, bool poll){
    //TODO:throw an exception if fd <= 0
     pd.fd = fd;
     //if user is providing fd,
     //add this to the underlying polling structure
     if(poll)
         ioh->open(pd);
}

void Connection::pollOpen(){
    ioh->reset(pd);
    ioh->open(pd);
}
void Connection::pollReset(){
    ioh->reset(pd);
}

int Connection::socket(int domain, int type, int protocol){

    //An active connection cannot be changed
    if(fd != -1) return -1;
    int sockfd = ::socket(domain, type | SOCK_NONBLOCK, protocol);
    if(sockfd != -1){
        fd = sockfd;
        pd.fd = sockfd;
    }
    return sockfd;
}

int Connection::listen(int backlog){
    assert(fd > 0);
    assert(fd == pd.fd);

    int res = ::listen(fd, backlog);
    //on success add to poll
    if(res == 0)    ioh->open(pd);
    return res;
}


int Connection::accept(Connection *conn, struct sockaddr *addr, socklen_t *addrlen){
    assert(fd != -1);
    assert(fd == pd.fd);
    //check connection queue for wating connetions
    //Set the fd as nonblocking
    int sockfd = ::accept4(fd, addr, addrlen, SOCK_NONBLOCK );
    while( (sockfd == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        //User level blocking using nonblocking io
        ioh->block(pd, IOHandler::Flag::UT_IOREAD);
        sockfd = ::accept4(fd, addr, addrlen, SOCK_NONBLOCK );
    }
    //otherwise return the result
    if(sockfd > 0){
        conn->setFD(sockfd);
        ioh->open(conn->pd);
    }
    return sockfd;
}

Connection* Connection::accept(struct sockaddr *addr, socklen_t *addrlen)
                                throw(std::system_error){
    assert(fd != -1);
    assert(fd == pd.fd);
    //check connection queue for wating connetions
    //Set the fd as nonblocking
    int sockfd = ::accept4(fd, addr, addrlen, SOCK_NONBLOCK );
    while( (sockfd == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        //User level blocking using nonblocking io
        ioh->block(pd, IOHandler::Flag::UT_IOREAD);
        sockfd = ::accept4(fd, addr, addrlen, SOCK_NONBLOCK );
    }
    //otherwise return the result
    if(sockfd > 0){
        return new Connection(sockfd);
    }else{
        throw std::system_error(EFAULT, std::system_category());
        return nullptr;
    }

}
int Connection::bind(const struct sockaddr *addr, socklen_t addrlen){
    assert(fd > 0);
    return ::bind(fd, addr, addrlen);
}

int Connection::connect(const struct sockaddr *addr,socklen_t addrlen){
    assert(fd > 0);
    int optval;
    uint optlen = sizeof(optval);
    ioh->open(pd);
    int res = ::connect(fd, addr, addrlen);
    while( (res == -1) && (errno == EINPROGRESS )){
        ioh->block(pd, IOHandler::Flag::UT_IOWRITE);
        ::getsockopt(fd,SOL_SOCKET, SO_ERROR, (void*)&optval, &optlen);

        if(optval ==0)
            res =0;
        else
            errno = optval;
    }
    return res;
}

ssize_t Connection::recv(void *buf, size_t len, int flags){
    assert(buf != nullptr);
    assert(fd != -1);

    ssize_t res = ::recv(fd, (void*)buf, len, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
           //User level blocking using nonblocking io
           ioh->block(pd, IOHandler::Flag::UT_IOREAD);
           res = ::recv(fd, buf, len, flags);
    }
    return res;
}
ssize_t Connection::recvfrom(void *buf, size_t len, int flags,
                            struct sockaddr *src_addr, socklen_t *addrlen){
    assert(buf != nullptr);
    assert(fd != -1);

    ssize_t res = ::recvfrom(fd, (void*)buf, len, flags, src_addr, addrlen);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
           //User level blocking using nonblocking io
           ioh->block(pd, IOHandler::Flag::UT_IOREAD);
           res = ::recvfrom(fd, buf, len, flags, src_addr, addrlen);
    }
    return res;
}

ssize_t Connection::recvmsg(int sockfd, struct msghdr *msg, int flags){
    assert(msg != nullptr);
    assert(fd != -1);

    ssize_t res = ::recvmsg(fd, msg, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        //User level blocking using nonblocking io
        ioh->block(pd, IOHandler::Flag::UT_IOREAD);
        res = ::recvmsg(fd, msg, flags);
    }
    return res;
}

int Connection::recvmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
             unsigned int flags, struct timespec *timeout){

    assert(msgvec != nullptr);
    assert(fd != -1);

    //TODO: is timeout applicable for the nonblocking call?
    int res = ::recvmmsg(fd, msgvec, vlen, flags, timeout);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        //User level blocking using nonblocking io
        ioh->block(pd, IOHandler::Flag::UT_IOREAD);
        res = ::recvmmsg(fd, msgvec, vlen, flags, timeout);
    }
    return res;
}
ssize_t Connection::send(const void *buf, size_t len, int flags){
    assert(buf != nullptr);
    assert(fd != -1);

    ssize_t res = ::send(fd, buf, len, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        ioh->block(pd, IOHandler::Flag::UT_IOWRITE);
        res = ::send(fd, buf, len, flags);
    }
    return res;
}
ssize_t Connection::sendto(int sockfd, const void *buf, size_t len, int flags,
            const struct sockaddr *dest_addr, socklen_t addrlen){
    assert(buf != nullptr);
    assert(dest_addr != nullptr);
    assert(fd != -1);

    ssize_t res = ::sendto(fd, buf, len, flags, dest_addr, addrlen);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        ioh->block(pd, IOHandler::Flag::UT_IOWRITE);
        res = ::sendto(fd, buf, len, flags, dest_addr, addrlen);
    }
    return res;

}
ssize_t Connection::sendmsg(const struct msghdr *msg, int flags){
    assert(fd != -1);

    ssize_t res = ::sendmsg(fd, msg, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        ioh->block(pd, IOHandler::Flag::UT_IOWRITE);
        res = ::sendmsg(fd, msg, flags);
    }
    return res;
}
int Connection::sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
             unsigned int flags){
    assert(msgvec != nullptr);
    assert(fd != -1);

    int res = ::sendmmsg(fd, msgvec, vlen, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        ioh->block(pd, IOHandler::Flag::UT_IOWRITE);
        res = ::sendmmsg(fd, msgvec, vlen, flags);
    }
    return res;

}
ssize_t Connection::read(void *buf, size_t count){
    assert(buf != nullptr);
    assert(fd != -1);

    ssize_t res = ::read(fd, (void*)buf, count);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
           //User level blocking using nonblocking io
           ioh->block(pd, IOHandler::Flag::UT_IOREAD);
           res = ::read(fd, buf, count);
    }
    return res;
}
ssize_t Connection::write(const void *buf, size_t count){
    assert(buf != nullptr);
    assert(fd != -1);

    ssize_t res = ::write(fd, buf, count);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        ioh->block(pd, IOHandler::Flag::UT_IOWRITE);
        res = ::write(fd, buf, count);
    }
    return res;
}

int Connection::close(){
    pd.closing.store(true);
    ioh->close(pd);
    return ::close(fd);
}

Connection::~Connection(){

}
