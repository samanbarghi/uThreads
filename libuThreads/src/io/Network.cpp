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
        fd = sockfd;
        init();
    }else{
        throw std::system_error(errno, std::system_category());
    }
}

void Connection::init(){
    //TODO:throw an exception if fd <= 0
     pd = IOHandler::iohandler.pollCache.getPollData();
     //no need to acquire the lock
     pd->fd = fd;
}

int Connection::socket(int domain, int type, int protocol){

    //An active connection cannot be changed
    if(fd != -1) return -1;
    int sockfd = ::socket(domain, type | SOCK_NONBLOCK, protocol);
    if(sockfd != -1){
        setFD(sockfd);
    }
    return sockfd;
}

int Connection::listen(int backlog){
    assert(fd > 0);

    int res = ::listen(fd, backlog);
    //on success add to poll
    return res;
}


int Connection::accept(Connection *conn, struct sockaddr *addr, socklen_t *addrlen){
    assert(fd != -1);
    //check connection queue for waiting connections
    //Set the fd as nonblocking
    int sockfd = ::accept4(fd, addr, addrlen, SOCK_NONBLOCK );
    while( (sockfd == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        //User level blocking using nonblocking io
        IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOREAD);
        sockfd = ::accept4(fd, addr, addrlen, SOCK_NONBLOCK );
    }
    //otherwise return the result
    if(sockfd > 0){
        conn->setFD(sockfd);
    }
    return sockfd;
}

Connection* Connection::accept(struct sockaddr *addr, socklen_t *addrlen)
                                throw(std::system_error){
    assert(fd != -1);
    //check connection queue for waiting connections
    //Set the fd as nonblocking
    int sockfd = ::accept4(fd, addr, addrlen, SOCK_NONBLOCK );
    while( (sockfd == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        //User level blocking using nonblocking io
        IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOREAD);
        sockfd = ::accept4(fd, addr, addrlen, SOCK_NONBLOCK );
    }
    //otherwise return the result
    if(sockfd > 0){
        return new Connection(sockfd);
    }else{
        throw std::system_error(errno, std::system_category());
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
    volatile int res = ::connect(fd, addr, addrlen);
    if( (res == -1) && (errno == EINPROGRESS )){
        IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOWRITE);
        ::getsockopt(fd,SOL_SOCKET, SO_ERROR, (void*)&optval, &optlen);

        if(optval ==0)
           return 0;
        else{
            errno = optval;
            return -1;
        }
    }
    return res;
}

void Connection::blockOnRead(){
        IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOREAD);
}

void Connection::blockOnWrite(){
        IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOWRITE);
}

ssize_t Connection::recv(void *buf, size_t len, int flags){
    assert(buf != nullptr);
    assert(fd != -1);

    ssize_t res = ::recv(fd, (void*)buf, len, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
           //User level blocking using nonblocking io
           IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOREAD);
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
           IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOREAD);
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
        IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOREAD);
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
        IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOREAD);
        res = ::recvmmsg(fd, msgvec, vlen, flags, timeout);
    }
    return res;
}
ssize_t Connection::send(const void *buf, size_t len, int flags){
    assert(buf != nullptr);
    assert(fd != -1);

    ssize_t res = ::send(fd, buf, len, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOWRITE);
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
        IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOWRITE);
        res = ::sendto(fd, buf, len, flags, dest_addr, addrlen);
    }
    return res;

}
ssize_t Connection::sendmsg(const struct msghdr *msg, int flags){
    assert(fd != -1);

    ssize_t res = ::sendmsg(fd, msg, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOWRITE);
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
        IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOWRITE);
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
           IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOREAD);
           res = ::read(fd, buf, count);
    }
    return res;
}
ssize_t Connection::write(const void *buf, size_t count){
    assert(buf != nullptr);
    assert(fd != -1);

    ssize_t res = ::write(fd, buf, count);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        IOHandler::iohandler.wait(*pd, IOHandler::Flag::UT_IOWRITE);
        res = ::write(fd, buf, count);
    }
    return res;
}

int Connection::close(){
    assert(pd != nullptr);
    IOHandler::iohandler.close(*pd);
    return ::close(fd);
}

Connection::~Connection(){

}
