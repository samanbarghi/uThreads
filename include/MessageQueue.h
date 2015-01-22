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
    BlockingQueue sendQueue;
    BlockingQueue recvQueue;

    bool internalSend(const Element& elem) {
        mlock.lock();
        while slowpath(slots == 0) {
            sendQueue.suspend(mlock);
            mlock.lock();
        }
        buffer.push(elem);
        slots -= 1;

        if fastpath(recvQueue.signal(mlock)) return true;
        mlock.unlock();
        return true;
    }

    bool internalRecv(Element& elem) {
        mlock.lock();
        while slowpath(slots >= buffer.max_size()) {
            recvQueue.suspend(mlock);
            mlock.lock();
        }
        elem = buffer.front();
        buffer.pop();
        slots += 1;

        if fastpath(sendQueue.signal(mlock)) return true;
        mlock.unlock();
        return true;
    }
public:
    explicit MessageQueue(size_t N = 0) :
            buffer(N), slots(buffer.max_size()) {
    }

    ~MessageQueue() {
        assert(buffer.empty());
        assert(slots == buffer.max_size());
    }
    bool send(const Element& elem) {
        return internalSend(elem);
    }

    bool recv(Element& elem) {
        return internalRecv(elem);
    }
    mword size() {
        return buffer.size();
    }
};

#endif /* MESSAGEQUEUE_H_ */
