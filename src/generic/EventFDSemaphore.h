/*
 * EventFDSemaphore.h
 *
 *  Created on: Jun 20, 2016
 *      Author: Saman Barghi
 */

#ifndef SRC_GENERIC_EVENTFDSEMAPHORE_H_
#define SRC_GENERIC_EVENTFDSEMAPHORE_H_
#include <sys/eventfd.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include "io/IOHandler.h"

class EventFDSemaphore{
private:
    PollData* _pd;
public:
    EventFDSemaphore(){

        int fd = eventfd(0, EFD_NONBLOCK);
        if(fd < 0)
        {
            std::cerr << "eventfd ERROR:" << errno << std::endl;
            exit(EXIT_FAILURE);
        }
        _pd = new PollData(fd);
        IOHandler::iohandler.openLT(*_pd);
    }
    ~EventFDSemaphore(){
        ::close(_pd->fd);
        delete _pd;
    }


    ssize_t post(){
        uint64_t value = 1;
        int res;
        while( (res = eventfd_write (_pd->fd, value)) == -1);
        return res;
    }

    /*
     * Should be a nonblocking read as the blocking
     * will be done with blocking poll
     */
    ssize_t wait(){
        uint64_t value =0;
        int res = 0;

        while( read(_pd->fd, &value, sizeof(value)) == -1 && errno == EAGAIN){
            IOHandler::iohandler.poll(-1, 0);
        };
        return value;
    }
};


#endif /* SRC_GENERIC_EVENTFDSEMAPHORE_H_ */
