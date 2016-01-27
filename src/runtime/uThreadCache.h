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
