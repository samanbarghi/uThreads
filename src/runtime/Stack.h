/*
 * Stack.h
 *
 *  Created on: Jan 11, 2016
 *      Author: Saman Barghi
 */
#pragma once
#include "generic/basics.h"
// initialize stack for invocation of 'func(arg1,arg2,arg3, arg4)'
class uThread;
extern "C" vaddr stackInit(vaddr stack, funcvoid1_t func, ptr_t arg1, ptr_t arg2, ptr_t arg3, ptr_t arg4);
extern "C" void stackSwitch(uThread* nextuThread, ptr_t args, vaddr* currSP, vaddr nextSP, void (*func)(uThread*,void*));
