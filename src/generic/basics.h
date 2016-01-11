/*
 * global.h
 *

 *  Created on: Oct 30, 2014
 *      Author: Saman Barghi
 */

#ifndef BASICS_H_
#define BASICS_H_
#include <stdint.h>
#include "KOS/generic/basics.h"

/*
* Constant variables
*/
static const size_t defaultStack = (8 * 1024);			    //8k stack size, TODO: determine what is the best stack size, or implement dynamic stack allocation

/*
 * Enumerations
 */
enum uThreadStatus {
	INITIALIZED,													//uThread is initialized
	READY,															//uThread is in a ReadyQueue
	RUNNING,														//uThread is Running
	YIELD,															//uThread Yields
	MIGRATE,														//Migrate to another cluster
	WAITING,														//uThread is in waiting mode
//	IOBLOCK,                                                        //uThread is blocked on IO
	TERMINATED														//uThread is done and should be terminated
};

/* polling flags */
enum pollingFlags {
    UT_IOREAD    = 1 << 0,                           //READ
    UT_IOWRITE   = 1 << 1                            //WRITE
};
#endif /* BASICS_H_ */
