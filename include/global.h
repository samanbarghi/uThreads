/*
 * global.h
 *
 *  Created on: Oct 30, 2014
 *      Author: Saman Barghi
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_
#include <stdint.h>


#define fastpath(x)  (__builtin_expect((bool(x)),true))
#define slowpath(x)  (__builtin_expect((bool(x)),false))
/*
 * Type definitions
 */
typedef void*			vaddr;										//memory address
typedef void* 			ptr_t;										//Pointer type
typedef unsigned short 	priority_t;									//Thread priority type
typedef uint64_t 		mword;
typedef  int64_t 		sword;

typedef void (*funcvoid0_t)();
typedef void (*funcvoid1_t)(ptr_t);
typedef void (*funcvoid2_t)(ptr_t, ptr_t);
typedef void (*funcvoid3_t)(ptr_t, ptr_t, ptr_t);
/*
* Constant variables
*/
static const unsigned int default_uthread_priority = 20;
static const unsigned int default_stack_size = (8 * 1024);			//8k stack size, TODO: determine what is the best stack size, or implement dynamic stack allocation
static const unsigned int defaultUT_stack_size = (128);				//defaultUT stackSize

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
	IOBLOCK,                                                        //uThread is blocked on IO
	TERMINATED														//uThread is done and should be terminated
};

/*
 * Assembly declarations
 */
class uThread;
// initialize stack for invocation of 'func(arg1,arg2,arg3, arg4)'
extern "C" mword stackInit(vaddr stack, funcvoid1_t func, ptr_t arg1, ptr_t arg2, ptr_t arg3, ptr_t arg4);
extern "C" void stackSwitch(uThread* nextuThread, void* args, vaddr* currSP, vaddr nextSP, void (*func)(uThread*,void*));

#define __noreturn   __attribute__((__noreturn__))
#define __packed     __attribute__((__packed__))



#endif /* GLOBAL_H_ */
