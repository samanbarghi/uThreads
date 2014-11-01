/*
 * Cluster.h
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#ifndef CLUSTER_H_
#define CLUSTER_H_
#include "ReadyQueue.h"
#include "kThread.h"


class Cluster {
private:
	kThread kt;					//There is at least one kernel thread per cluster
public:
	Cluster();
	virtual ~Cluster();
	ReadyQueue readyQueue;		//There is at least one ready queue per cluster
};


#endif /* CLUSTER_H_ */
