/*
 * uThreadLib.h
 *
 *  Created on: Oct 23, 2014
 *      Author: Saman Barghi
 */

#ifndef UTHREADLIB_H_
#define UTHREADLIB_H_
#include <stdio.h>
#include <list>
#include "Cluster.h"


class uThreadLib {
public:
	uThreadLib();
	virtual ~uThreadLib();

	static void run(void*) __noreturn;
	static void postFunc(vaddr*, vaddr) __noreturn;
};


#endif /* UTHREADLIB_H_ */
