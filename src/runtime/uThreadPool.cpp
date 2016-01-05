/*
 * uThreadPool.cpp
 *
 *  Created on: Oct 2, 2015
 *      Author: Saman Barghi
 */

#include <iostream>
#include "uThreadPool.h"

uThreadPool::uThreadPool() {
    //atomic operations
    totalnumuThreads    = 0;
    idleuThreads        = 0;



}

uThreadPool::~uThreadPool() {
}


void uThreadPool::uThreadExecute(funcvoid1_t func, void* args, Cluster* cluster){


    //check if there is idle threads
    if(idleuThreads.load() == 0){

        Argument* runArgs = new Argument(func, args, this);
        //create a new uThread
        uThread::create((funcvoid1_t)(this->run), (void*)runArgs, cluster);
        //atomically increase the total number of threads
        totalnumuThreads++;
    }else{
       //if there are uThreads waiting wake one up to continue
       this->mutex.acquire();
       taskList.emplace(func, args);
       this->cv.signal(mutex);
    }


}

void uThreadPool::run(void* args){
    Argument* funcArg = (Argument*) args;

    funcvoid1_t func        =   funcArg->func;
    void* fargs             =   funcArg->args;
    uThreadPool* utp        =   funcArg->utp;
    delete(funcArg);

    //run the first passed function
    func(fargs);

    //This thread is about to become idle
    utp->idleuThreads++;
    //now block on cv in a loop until the next function is available
    while(true){
        utp->mutex.acquire();

        while(utp->taskList.empty()){
            utp->cv.wait(utp->mutex);
        }
        utp->idleuThreads--;
        auto task   = utp->taskList.front();
        utp->taskList.pop();
        utp->mutex.release();

        //run the function
        task.first(task.second);
        utp->idleuThreads++;
    }
}
