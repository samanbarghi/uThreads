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
#include <condition_variable>
#include "global.h"
#include "EmbeddedList.h"

class ReadyQueue {
private:
//	std::priority_queue<uThread*, std::vector<uThread*>, CompareuThread> priorityQueue;
	EmbeddedList<uThread> queue;
	std::mutex mtx;
	std::condition_variable cv;
public:
	ReadyQueue(){};
	virtual ~ReadyQueue(){};

	uThread* pop(){
		uThread* ut = nullptr;
		std::unique_lock<std::mutex> mlock(mtx);
		if(!queue.empty()){
			ut = queue.front();
			queue.pop_front();
		}
		mlock.unlock();
		return ut;
	}
	uThread* cvPop(){									//Pop with condition variable
		std::unique_lock<std::mutex> mlock(mtx);
		while(queue.empty()){cv.wait(mlock);}
		uThread* ut = queue.front();
		queue.pop_front();
		mlock.unlock();
		return ut;
	}
	void push(uThread* ut){
		std::unique_lock<std::mutex> mlock(mtx);
		queue.push_back(ut);
		mlock.unlock();
		cv.notify_one();
	}
	bool empty() const{return queue.empty();}
};

class Cluster {
	friend class kThread;
private:
	ReadyQueue readyQueue;								//There is one ready queue per cluster

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
	uThread* tryGetWork();									//Get a unit of work from the ready queue
	uThread* getWork();							//Get a unit of work or if not available sleep till there is one


};


#endif /* CLUSTER_H_ */
