/*******************************************************************************
 *     Copyright © 2015, 2016 Saman Barghi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

#ifndef UTHREADS_BASICS_H_
#define UTHREADS_BASICS_H_

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
static const size_t defaultStackSize        = (8 * 1024);           //8k stack size, TODO: determine what is the best stack size, or implement dynamic stack allocation
static const size_t defaultuThreadCacheSize = 1000;                 //Maximum number of uThreads that should be cached


/*
 * Assembly declarations
 */
#else
#error undefined platform: only __linux__ supported at this time
#endif

#endif /* UTHREADS_BASICS_H_*/
