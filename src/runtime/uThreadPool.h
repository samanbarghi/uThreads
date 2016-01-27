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

#ifndef UTHREADS_UTHREAD_POOL_H_
#define UTHREADS_UTHREAD_POOL_H_

#include <atomic>
#include <queue>
#include "BlockingSync.h"
#include "uThread.h"

class uThreadPool {
private:
    struct Argument{                                             //This struct is replaced by the args the first time a uThread is created
        funcvoid1_t         func;
        void*               args;

        uThreadPool*        utp;

        Argument(funcvoid1_t func, void* args, uThreadPool* utp): func(func), args(args), utp(utp){};

    };

    std::atomic<unsigned int>    totalnumuThreads;                //Total number of uThreads belonging to this thread pool
    std::atomic<unsigned int>    idleuThreads;                   //idle uThreads

    Mutex       mutex;                                          //Mutex to protect data structures in thread pool
    ConditionVariable   cv;                                     //condition variable to block idle uThreads on

    std::queue<std::pair<funcvoid1_t, void*>>  taskList;

    static void run(void*);


public:
    uThreadPool();
    virtual ~uThreadPool();

   void uThreadExecute(funcvoid1_t, void*, Cluster&);

};

#endif /* UTHREADS_UTHREAD_POOL_H_ */
