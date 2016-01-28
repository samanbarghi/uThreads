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
//TODO: Make this independent of the single instance of IOHandler

void Connection::init(int fd, bool poll){
    //TODO:throw an exception if fd <= 0
     pd.fd = fd;
     //if user is providing fd,
     //add this to the underlying polling structure
     if(poll)
         ioh->open(this->pd);
}

void Connection::pollOpen(){
    ioh->reset(this->pd);
    ioh->open(this->pd);
}
void Connection::pollReset(){
    ioh->reset(this->pd);
}

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
    if(res == 0)    ioh->open(this->pd);
    return res;
}

int Connection::accept(Connection *conn, struct sockaddr *addr, socklen_t *addrlen){
    assert(this->fd != -1);
    //check connection queue for wating connetions
    //Set the fd as nonblocking
    int sockfd = ::accept4(this->fd, addr, addrlen, SOCK_NONBLOCK );
    while( (sockfd == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        //User level blocking using nonblocking io
        ioh->block(this->pd, IOHandler::Flag::UT_IOREAD);
        sockfd = ::accept4(this->fd, addr, addrlen, SOCK_NONBLOCK );
    }
    //otherwise return the result
    if(sockfd > 0){
        conn->setFD(sockfd);
        ioh->open(conn->pd);
    }
    return sockfd;
}

ssize_t Connection::recv(void *buf, size_t len, int flags){
    assert(buf != nullptr);
    assert(this->fd != -1);

    ssize_t res = ::recv(this->fd, (void*)buf, len, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
           //User level blocking using nonblocking io
           ioh->block(this->pd, IOHandler::Flag::UT_IOREAD);
           res = ::recv(this->fd, buf, len, flags);
    }
    return res;
}
ssize_t Connection::recvfrom(void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){
    assert(buf != nullptr);
    assert(this->fd != -1);

    ssize_t res = ::recvfrom(this->fd, (void*)buf, len, flags, src_addr, addrlen);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
           //User level blocking using nonblocking io
           ioh->block(this->pd, IOHandler::Flag::UT_IOREAD);
           res = ::recvfrom(this->fd, buf, len, flags, src_addr, addrlen);
    }
    return res;
}
ssize_t Connection::send(const void *buf, size_t len, int flags){
    assert(buf != nullptr);
    assert(this->fd != -1);

    ssize_t res = ::send(this->fd, buf, len, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        ioh->block(this->pd, IOHandler::Flag::UT_IOWRITE);
        res = ::send(this->fd, buf, len, flags);
    }
    return res;
}
ssize_t Connection::sendmsg(const struct msghdr *msg, int flags){
    assert(this->fd != -1);

    ssize_t res = ::sendmsg(this->fd, msg, flags);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        ioh->block(this->pd, IOHandler::Flag::UT_IOWRITE);
        res = ::sendmsg(this->fd, msg, flags);
    }
    return res;
}

ssize_t Connection::read(void *buf, size_t count){
    assert(buf != nullptr);
    assert(this->fd != -1);

    ssize_t res = ::read(this->fd, (void*)buf, count);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
           //User level blocking using nonblocking io
           ioh->block(this->pd, IOHandler::Flag::UT_IOREAD);
           res = ::read(this->fd, buf, count);
    }
    return res;
}
ssize_t Connection::write(const void *buf, size_t count){
    assert(buf != nullptr);
    assert(this->fd != -1);

    ssize_t res = ::write(this->fd, buf, count);
    while( (res == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)){
        ioh->block(this->pd, IOHandler::Flag::UT_IOWRITE);
        res = ::write(this->fd, buf, count);
    }
    return res;
}

int Connection::close(){
    this->pd.closing.store(true);
    ioh->close(this->pd);
    return ::close(this->fd);
}

Connection::~Connection(){

}
