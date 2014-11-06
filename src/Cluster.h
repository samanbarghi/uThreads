/*
 * Cluster.h
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#ifndef CLUSTER_H_
#define CLUSTER_H_
#include "ReadyQueue.h"
#include "global.h"
#include <vector>

class Cluster {
	friend class kThread;
private:
	ReadyQueue readyQueue;								//There is at least one ready queue per cluster

public:
	Cluster();
	virtual ~Cluster();

	Cluster(const Cluster&) = delete;
	const Cluster& operator=(const Cluster&) = delete;

	static Cluster	defaultCluster;						//Default cluster
	static std::vector<Cluster*> clusters;				//List of all clusters

	static void invoke(funcvoid1_t, void*) __noreturn;	//Function to invoke the run function of a uThread
	void uThreadStart(uThread*);						//Put ut in the ready queue for the first time to be picked up by kThread later
	uThread* getWork();									//Get a unit of work from the ready queue
	uThread* getWorkOrWait();							//Get a unit of work or if not available sleep till there is one


};


#endif /* CLUSTER_H_ */
