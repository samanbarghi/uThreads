/*
 * uThreadCache.h
 *
 *  Created on: Jan 12, 2016
 *      Author: Saman Barghi
 */

#pragma once
#include <mutex>
#include "generic/IntrusiveContainers.h"
#include "uThread.h"

//caching interface for uThreads
class uThreadCache {

    //This is a singleton class
    static uThreadCache *single_instance;
    uThreadCache(size_t size) : count(0), size(size){};

    std::mutex mtx;             //global mutex for protecting the underlying data structure
    IntrusiveList<uThread> stack;
    size_t count;
    size_t size;

public:
    static uThreadCache* instance(){
        if(!single_instance)
            single_instance = new uThreadCache(defaultuThreadCacheSize);
        return single_instance;
    }

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
