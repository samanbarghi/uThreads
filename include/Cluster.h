/*
 * Cluster.h
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#ifndef CLUSTER_H_
#define CLUSTER_H_
#include <vector>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include <thread>
#include "global.h"
#include "EmbeddedList.h"

class ReadyQueue {
	friend class kThread;
	friend class Cluster;
private:
//	std::priority_queue<uThread*, std::vector<uThread*>, CompareuThread> priorityQueue;
	EmbeddedList<uThread> queue;
	std::mutex mtx;
	std::condition_variable cv;
	volatile mword	size;
public:
	ReadyQueue(): size(0){};
	virtual ~ReadyQueue(){};

	uThread* tryPop(){								//Try to pop one item, or return null
		uThread* ut = nullptr;
		std::unique_lock<std::mutex> mlock(mtx);
		if(!queue.empty()){
			ut = queue.front();
			queue.pop_front();
			size--;
		}
		mlock.unlock();
		return ut;
	}

	void tryPopMany(EmbeddedList<uThread> *nqueue, mword numkt){//Try to pop ReadyQueueSize/#kThreads in cluster from the ready Queue
		std::unique_lock<std::mutex> mlock(mtx);
        if(size == 0) // There is no uThreads
        {
            mlock.unlock();
            return;
        }
		//TODO: is 1 (fall back to one task per each call) is a good number or should we used size%numkt
		int popnum = (size/numkt) ? (size/numkt) : 1; //To avoid emptying the queue and not leaving enough work for other kThreads only move a portion of the queue
        //std::cout << "Size: " << size << " | numkt: " << numkt << " | PopNum: " << popnum << std::endl;

		uThread* ut;
		for( ; popnum > 0 && !queue.empty(); popnum--){
			ut = queue.front();
			queue.pop_front();
			nqueue->push_back(ut);
			size--;
		}
		mlock.unlock();
	}


	void pop(EmbeddedList<uThread> *nqueue, mword numkt){									//Pop with condition variable
		std::unique_lock<std::mutex> mlock(mtx);
		while(queue.empty()){cv.wait(mlock);}

		int popnum = (size/numkt) ? (size/numkt) : 1; //To avoid emptying the queue and not leaving enough work for other kThreads only move a portion of the queue

		uThread* ut;
		for( ; popnum > 0 && !queue.empty(); popnum--){
			ut = queue.front();
			queue.pop_front();
			nqueue->push_back(ut);
			size--;
		}
		mlock.unlock();
	}

	void push(uThread* ut){
		std::unique_lock<std::mutex> mlock(mtx);
		queue.push_back(ut);
		size++;
		mlock.unlock();
		cv.notify_one();
	}
	bool empty() const{return queue.empty();}
};

class Cluster {
	friend class kThread;
private:
	ReadyQueue readyQueue;								//There is one ready queue per cluster
	mword	numberOfkThreads;							//Number of kThreads in this Cluster

public:
	Cluster();
	virtual ~Cluster();

	Cluster(const Cluster&) = delete;
	const Cluster& operator=(const Cluster&) = delete;

	static Cluster	defaultCluster;						//Default cluster
	static Cluster	syscallCluster;						//Syscall cluster
	static std::vector<Cluster*> clusters;				//List of all clusters

	static void invoke(funcvoid1_t, void*) __noreturn;	//Function to invoke the run function of a uThread
	void uThreadSchedule(uThread*);						//Put ut in the ready queue to be picked up by kThread
	uThread* tryGetWork();								//Get a unit of work from the ready queue
	void tryGetWorks(EmbeddedList<uThread>*);			//Get as many uThreads as possible from the readyQueue and move them to local queue
	void getWork(EmbeddedList<uThread>*);									//Get a unit of work or if not available sleep till there is one


};


#endif /* CLUSTER_H_ */
