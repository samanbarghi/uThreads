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

#ifndef UTHREADS_INCLUDE_NETWORK_H_
#define UTHREADS_INCLUDE_NETWORK_H_
#include "io/IOHandler.h"

class Connection{
    friend IOHandler;
private:
    PollData pd;
    int fd = -1;             //related file descriptor
    void init(int fd, bool poll);

    Connection(int fd, bool poll): fd(fd){init(fd, poll);};


    //set
    void setFD(int fd){
        this->fd = fd;
        this->pd.fd = fd;
    }

    IOHandler* ioh;
public:

    Connection(): fd(-1){
        ioh = uThread::currentUThread()->getCurrentCluster().iohandler;
    }
    Connection(int fd) : fd(fd){
        ioh = uThread::currentUThread()->getCurrentCluster().iohandler;
        init(fd, true);
    };
    ~Connection();

    void pollOpen();
    void pollReset();

    //IO functions
    ssize_t recv(void *buf, size_t len, int flags);
    ssize_t recvfrom(void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
    ssize_t send(const void *buf, size_t len, int flags);
    ssize_t sendmsg(const struct msghdr *msg, int flags);

    ssize_t read(void *buf, size_t count);
    ssize_t write(const void *buf, size_t count);

    int accept(Connection *conn, struct sockaddr *addr, socklen_t *addrlen);
    int socket(int domain, int type, int protocol);
    int listen(int backlog);
    int close();

    //getters
    int getFd() const {
        return fd;
    }

};




#endif /* UTHREADS_INCLUDE_NETWORK_H_ */
