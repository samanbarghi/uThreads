/*
 * Cluster.h
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#ifndef CLUSTER_H_
#define CLUSTER_H_
#include "ReadyQueue.h"


class Cluster {
private:
public:
	Cluster();
	virtual ~Cluster();
	ReadyQueue readyQueue;		//There is at least one ready queue per cluster
};


#endif /* CLUSTER_H_ */
