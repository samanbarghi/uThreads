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

#ifndef UTHREADS_CACHE_H_
#define UTHREADS_CACHE_H_

#include <mutex>
#include "../generic/IntrusiveContainers.h"
#include "uThread.h"

/**
 * @class uThreadCache
 * @brief Data structure to cache uThreads
 *
 * uThreadCache is a linked list of uThreads using and intrusive container
 * to cache all terminated uThreads. Instead of destroying the memory allocated
 * for the stack, simply reset the stack pointer and push it to the cache.
 */
class uThreadCache {

private:
    //global mutex for protecting the underlying data structure
    std::mutex mtx;
    IntrusiveStack<uThread> stack;
    size_t count;
    size_t size;

public:
    uThreadCache(size_t size = defaultuThreadCacheSize) : count(0), size(size){};
    ~uThreadCache(){}

    /**
     * @brief adds a uThread to the cache
     * @param ut pointer to a uThread
     * @return size of the cache if push was successful or -1 if not
     *
     * This function tries to push a uThread into the cache structure. If
     * the cache is full or the mutex cannot be acquired immediately the
     * operation has failed and the function returns -1. Otherwise, it adds the
     * uThread to the list and return the size of the cache.
     */
    ssize_t push(uThread* ut){
        std::unique_lock<std::mutex> mlock(mtx, std::try_to_lock);
        //do not block just to grab this lock
        if(!mlock.owns_lock() || count == size) return -1;
        ut->reset();
        stack.push(*ut);
        return ++count;
    }

    /**
     * @brief pop a uThread from the list in FIFO order and return it
     * @return nullptr on failure, or a pointer to a uThread on success
     */
    uThread* pop(){
        std::unique_lock<std::mutex> mlock(mtx, std::try_to_lock);
        //do not block just to grab this lock
        if(!mlock.owns_lock() || count == 0) return nullptr;
        count--;
        return stack.pop();
    }
};

#endif /* UTHREADS_CACHE_H_ */
