/*
 * uThreadCache.h
 *
 *  Created on: Jan 12, 2016
 *      Author: Saman Barghi
 */

#ifndef UTHREADS_CACHE_H_
#define UTHREADS_CACHE_H_

#include <mutex>
#include "generic/IntrusiveContainers.h"
#include "uThread.h"

//caching interface for uThreads
class uThreadCache {

    std::mutex mtx;             //global mutex for protecting the underlying data structure
    IntrusiveList<uThread> stack;
    size_t count;
    size_t size;

public:
    uThreadCache(size_t size = defaultuThreadCacheSize) : count(0), size(size){};
    ~uThreadCache(){}

    ssize_t push(uThread* ut){
        std::unique_lock<std::mutex> mlock(mtx, std::try_to_lock);
        if(!mlock.owns_lock() || count == size) return -1;                      //do not block just to grab this lock
        ut->reset();
        stack.push_back(*ut);
        return ++count;
    }

    uThread* pop(){
        std::unique_lock<std::mutex> mlock(mtx, std::try_to_lock);
        if(!mlock.owns_lock() || count == 0) return nullptr;                    //do not block just to grab this lock
        uThread* ut = stack.back();
        stack.pop_back();
        count--;
        return ut;
    }
};

#endif /* UTHREADS_CACHE_H_ */
