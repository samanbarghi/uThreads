/*
 * Stack.h
 *
 *  Created on: Jan 5, 2016
 *      Author: Saman Barghi
 */

#ifndef SRC_RUNTIME_STACK_H_
#define SRC_RUNTIME_STACK_H_
#include "generic/basics.h"

class uThread;
// initialize stack for invocation of 'func(arg1,arg2,arg3, arg4)'
extern "C" mword stackInit(ptr_t stack, funcvoid1_t func, ptr_t arg1, ptr_t arg2, ptr_t arg3, ptr_t arg4);
extern "C" void stackSwitch(uThread* nextuThread, void* args, ptr_t* currSP, ptr_t nextSP, void (*func)(uThread*,void*));

#endif /* SRC_RUNTIME_STACK_H_ */
