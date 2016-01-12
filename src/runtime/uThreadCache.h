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

    std::mutex mtx;             //global mutex for protecting the underlying data structure
    IntrusiveList<uThread> stack;
    size_t count;
    size_t size;

public:
    uThreadCache(size_t size = defaultuThreadCacheSize) : count(0), size(size){};
    ~uThreadCache(){
//        std::lock_guard<std::mutex> lock(mtx) ;
//        printf("Size: %zu\n", count);
//        for(size_t i = 0; i < count && !stack.empty(); i++){
//
//           printf("Size: %zu\n", i);
//           uThread* ut = stack.front();
//           stack.pop_front();
//           if(ut != uThread::initUT)
//               ut->destory(true);
//        }
    }

    ssize_t push(uThread* ut){
        std::lock_guard<std::mutex> lock(mtx);
        if(count == size) return -1;
        ut->reset();
        stack.push_back(*ut);
        return ++count;
    }

    uThread* pop(){
        std::lock_guard<std::mutex> lock(mtx);
        if(count == 0) return nullptr;
        uThread* ut = stack.front();
        stack.pop_front();
        count--;
        return ut;
    }
};
