/*
 * Network.h
 *
 *  Created on: Oct 21, 2015
 *      Author: Saman Barghi
 */

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
public:

    Connection(): fd(-1){}
    Connection(int fd) : fd(fd){ init(fd, true);};
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
