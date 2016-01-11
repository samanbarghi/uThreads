/*
 * basics.h
 *
 *  Created on: Oct 30, 2014
 *      Author: Saman Barghi
 */

#pragma once
#include <stdint.h>

#if defined(__linux__)

#define fastpath(x)  (__builtin_expect((bool(x)),true))
#define slowpath(x)  (__builtin_expect((bool(x)),false))

#define __section(x) __attribute__((__section__(x)))
#define __aligned(x) __attribute__((__aligned__(x)))
#define __packed     __attribute__((__packed__))

#define __finline    __attribute__((__always_inline__))
#define __ninline    __attribute__((__noinline__))
#define __noreturn   __attribute__((__noreturn__))
#define __useresult  __attribute__((__warn_unused_result__))


/*
 * Type definitions
 */
#include "linuxtypes.h"
/*
* Constant variables
*/
static const unsigned int default_stack_size = (8 * 1024);			//8k stack size, TODO: determine what is the best stack size, or implement dynamic stack allocation

/* polling flags */
enum pollingFlags {
    UT_IOREAD    = 1 << 0,                           //READ
    UT_IOWRITE   = 1 << 1                            //WRITE
};

/*
 * Assembly declarations
 */
#else
#error undefined platform: only __linux__ supported at this time
#endif

