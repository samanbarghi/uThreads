/*
 * linuxtypes.h
 *
 *  Created on: Jan 11, 2016
 *      Author: Saman Barghi
 */

#ifndef UTHREADS_LINUX_TYPES_H_
#define UTHREADS_LINUX_TYPES_H_

#include <cstddef>
#include <cstdint>
#if defined (__x86_64__)

typedef uint64_t        mword;
typedef  int64_t        sword;

typedef intptr_t        vaddr;                                  //memory address
typedef void*           ptr_t;                                  //Pointer type

typedef void (*funcvoid0_t)();
typedef void (*funcvoid1_t)(ptr_t);
typedef void (*funcvoid2_t)(ptr_t, ptr_t);
typedef void (*funcvoid3_t)(ptr_t, ptr_t, ptr_t);

#else
#error unsupported architecture: only __x86_64__ supported at this time
#endif

#endif /* UTHREADS_LINUX_TYPES_H_ */
