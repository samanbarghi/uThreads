/*
 * Network.h
 *
 *  Created on: Oct 21, 2015
 *      Author: Saman Barghi
 */

#ifndef INCLUDE_NETWORK_H_
#define INCLUDE_NETWORK_H_
#include "IOHandler.h"

class Connection{
    friend IOHandler;
private:
    PollData pd;
    int fd = -1;             //related file descriptor
    Connection(int fd, bool poll);

    //set
    void setFD(int fd){
        this->fd = fd;
        this->pd.fd = fd;
    }
public:

    Connection(): fd(-1){}
    ~Connection();

    //IO functions
    ssize_t recv(void *buf, size_t len, int flags);
    ssize_t send(const void *buf, size_t len, int flags);
    int accept(Connection *conn, struct sockaddr *addr, socklen_t *addrlen);
    int socket(int domain, int type, int protocol);
    int listen(int backlog);
    int close();

    //getters
    int getFd() const {
        return fd;
    }

};




#endif /* INCLUDE_NETWORK_H_ */
