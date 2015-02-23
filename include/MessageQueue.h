/*
 * MessageQueue.h
 *
 *  Created on: Jan 21, 2015
 *      Author: Saman Barghi
 */

#ifndef MESSAGEQUEUE_H_
#define MESSAGEQUEUE_H_
#include <mutex>
#include "BlockingSync.h"

template<typename Buffer>
class MessageQueue {
private:
    typedef typename Buffer::Element Element;
    std::mutex mlock;
    Buffer buffer;
    size_t slots;                   //available slots
    Mutex mtx;
    ConditionVariable sendCV;
    ConditionVariable recvCV;
    BlockingQueue sendQueue;
    BlockingQueue recvQueue;

    int internalSend(const Element& elem) {
        //mtx.acquire();
        mlock.lock();
        while slowpath(slots == 0) {sendQueue.suspend(mlock);mlock.lock();}
        buffer.push(elem);
        slots -= 1;

        //recvCV.signal(mtx);
        if(recvQueue.signal(mlock, kThread::currentKT->currentUT)) return true;
        mlock.unlock();
        return (buffer.max_size()-slots);
    }

    bool internalRecv(Element& elem) {
        //mtx.acquire();
        mlock.lock();
        while slowpath(slots >= buffer.max_size()) {recvQueue.suspend(mlock);mlock.lock();}
        elem = buffer.front();
        buffer.pop();
        slots += 1;

        if(sendQueue.signal(mlock, kThread::currentKT->currentUT)) return true;
        mlock.unlock();

        return true;
    }

    //Return maximum 10 elements
    int internalRecvMany(Element* elements) {
        int count = 0;
        //mtx.acquire();
        mlock.lock();
        while slowpath(slots >= buffer.max_size()) {recvQueue.suspend(mlock);mlock.lock();}
        //while slowpath(slots >= buffer.max_size()) {recvCV.wait(mtx);}
        //grab as many items as you can
        while(slots < buffer.max_size() && count < 5){
            elements[count++] = buffer.front();
            buffer.pop();
            slots += 1;
        }

        if(sendQueue.signal(mlock, kThread::currentKT->currentUT)) return true;
        mlock.unlock();

        return count;
    }

public:
    explicit MessageQueue(size_t N = 0) :
            buffer(N), slots(buffer.max_size()) {
    }

    ~MessageQueue() {
        assert(buffer.empty());
        assert(slots == buffer.max_size());
    }
    int send(const Element& elem) {
        return internalSend(elem);
    }

    bool recv(Element& elem) {
        return internalRecv(elem);
    }
    int recvMany(Element* elem){
        return internalRecvMany(elem);
    }
    mword size() {
        return buffer.size();
    }
};

#endif /* MESSAGEQUEUE_H_ */
