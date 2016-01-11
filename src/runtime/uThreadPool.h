/*
 * uThreadPool.h
 *
 *  Created on: Oct 2, 2015
 *      Author: Saman Barghi
 */

#pragma once
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

   void uThreadExecute(funcvoid1_t, void*, Cluster*);

};
