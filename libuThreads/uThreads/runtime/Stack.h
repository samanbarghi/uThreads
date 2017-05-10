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

#ifndef UTHREADS_STACK_H_
#define UTHREADS_STACK_H_

#include "../generic/basics.h"
// initialize stack for invocation of 'func(arg1,arg2,arg3, arg4)'
namespace uThreads {
namespace runtime {
class uThread;
}
}
using uThreads::runtime::uThread;

extern "C" vaddr stackInit(vaddr stack, ptr_t func, ptr_t arg1,
                           ptr_t arg2, ptr_t arg3, ptr_t arg4);
extern "C" void
stackSwitch(uThread *nextuThread, ptr_t args, vaddr *currSP,
            vaddr nextSP, void (*func)(uThread *, void *));


#endif /* UTHREADS_STACK_H_*/
