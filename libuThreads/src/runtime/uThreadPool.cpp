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

#include <iostream>
#include "uThreadPool.h"

uThreadPool::uThreadPool() {
    //atomic operations
    totalnumuThreads    = 0;
    idleuThreads        = 0;
}

uThreadPool::~uThreadPool() {
}


void uThreadPool::uThreadExecute(funcvoid1_t func, void* args, Cluster& cluster){


    //check if there is idle threads
    if(idleuThreads.load() == 0){

        Argument* runArgs = new Argument(func, args, this);
        //create a new uThread

        uThread* ut = uThread::create();
        ut->start(cluster, (ptr_t)(this->run), (void*)runArgs);
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
