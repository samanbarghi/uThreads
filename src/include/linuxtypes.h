/*
 * linuxtypes.h
 *
 *  Created on: Jan 5, 2016
 *      Author: Saman Barghi
 */

#ifndef SRC_GENERIC_LINUXTYPES_H_
#define SRC_GENERIC_LINUXTYPES_H_

#include <cstddef>
#include <cstdint>
#if defined(__x86_64__)

typedef uint64_t mword;
typedef  int64_t sword;

typedef mword vaddr;
typedef mword paddr;

#else
#error unsupported architecture: only __x86_64__ supported at this time
#endif




#endif /* SRC_GENERIC_LINUXTYPES_H_ */
