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

#ifndef UTHREADS_KTHREADS_H_
#define UTHREADS_KTHREADS_H_

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

#include "generic/basics.h"
#include "runtime/Cluster.h"
#include "runtime/uThread.h"


class kThread : public IntrusiveList<kThread>::Link {
	friend class uThread;
	friend class Cluster;
	friend class ReadyQueue;
	friend class IOHandler;

private:
	kThread(bool);                          //This is only for the initial kThread
	kThread(Cluster&, std::function<void(ptr_t)>, ptr_t); //This constructor is used to create kThreads that runs a single uThread with the assigned function
	std::thread threadSelf;                //pthread related to the current thread
	uThread* mainUT;                        //Each kThread has a default uThread that is used when there is no work available

	static kThread defaultKT;               //default main thread of the application
	/* make user create the kernel thread for ths syscalls as required */

	Cluster* localCluster;					//Pointer to the cluster that provides jobs for this kThread

	static __thread IntrusiveList<uThread>* ktReadyQueue;	//internal readyQueue for kThread, to avoid locking and unlocking the cluster ready queue
	std::condition_variable   cv;                           //condition variable to be used by ready queue in the Cluster
	bool cv_flag;                                           //A flag to track the state of kThread on CV stack

	void run();                         //The run function for the thread.
	void runWithFunc(std::function<void(ptr_t)>, ptr_t);
	void initialize(bool);              //Initialization function for kThread
	static inline void postSwitchFunc(uThread*, void*) __noreturn;

    void initialSynchronization();

public:
    //TODO: add a function to create multiple kThreads on a given cluster
	kThread();                              //Create a kThread on defaultCluster
	kThread(Cluster&);                      //Create a single kThread on cluster
	virtual ~kThread();

	uThread* currentUT;						//Pointer to the current running ut
	static __thread kThread* currentKT;

	static std::atomic_uint totalNumberofKTs;

	void switchContext(uThread*,void* args = nullptr);			//Put current uThread in ready Queue and run the passed uThread
	void switchContext(void* args = nullptr);					//Put current uThread in readyQueue and pop a new uThread to run
	static void defaultRun(void*) __noreturn;


	std::thread::native_handle_type getThreadNativeHandle();
	std::thread::id	getThreadID();
};

#endif /* UTHREADS_KTHREADS_H_ */
