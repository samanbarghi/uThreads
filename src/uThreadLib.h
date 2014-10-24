/*
 * uThreadLib.h
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#ifndef UTHREADLIB_H_
#define UTHREADLIB_H_
#include <stdio.h>
#include <vector>
#include "Cluster.h"


class uThreadLib {
private:
	std::vector<Cluster> clusters;
public:
	uThreadLib();
	virtual ~uThreadLib();
};


#endif /* UTHREADLIB_H_ */
