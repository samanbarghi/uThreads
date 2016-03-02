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

#include "BlockingSync.h"
#include "uThread.h"

bool BlockingQueue::suspend(std::mutex& lock) {

	IntrusiveList<uThread>* lqueue = &(this->queue);
	auto lambda([&, this](){
	    this->queue.push_back(*uThread::currentUThread());
	    lock.unlock();
	});
	std::function<void()> f(std::cref(lambda));
    postSwitchStruct* pss = new postSwitchStruct((funcvoid2_t)BlockingQueue::postSwitchFunc, (void*)&f);
    uThread::currentUThread()->suspend(*pss);
    //TODO: we do not have any cancellation yet, so this line will not be reached at all
    return true;
}

bool BlockingQueue::suspend(Mutex& mutex) {
	auto lambda([&, this](){
	        this->queue.push_back(*uThread::currentUThread());
	        mutex.release();
	    });
	    std::function<void()> f(std::cref(lambda));
        postSwitchStruct* pss = new postSwitchStruct((funcvoid2_t)BlockingQueue::postSwitchFunc, (void*)&f);
        uThread::currentUThread()->suspend(*pss);
    return true;
}

bool BlockingQueue::signal(std::mutex& lock, uThread*& owner) {
    //TODO: handle cancellation
    //Fetch one thread and put it back to ready queue
    if (queue.front() != queue.fence()) {
        owner = queue.front();//FIFO?
        queue.pop_front();
        lock.unlock();
        owner->resume();
        return true;
    }
    return false;
}

bool BlockingQueue::signal(Mutex& mutex) {
    //TODO: handle cancellation
    uThread* owner = nullptr;
    //Fetch one thread and put it back to ready queue
    if (!queue.empty()) {
        owner = queue.front();//FIFO?
        queue.pop_front();
        mutex.release();
        owner->resume();
        return true;
    }
    return false;
}

void BlockingQueue::signalAll(Mutex& mutex) {

    uThread* ut = queue.front();
    for (;;) {
        if (slowpath(ut == queue.fence())) break;
        queue.remove(*ut);
        ut->resume();
        ut = queue.front();
    }
    mutex.release();
}

uThread* BlockingQueue::getCurrentUThread(){
    return uThread::currentUThread();
}
