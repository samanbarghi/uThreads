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

#ifndef SRC_RUNTIME_SCHEDULERS_SCHEDULER_H_
#define SRC_RUNTIME_SCHEDULERS_SCHEDULER_H_

#ifndef SCHEDULERNO
#define SCHEDULERNO 2
#endif

#if SCHEDULERNO == 1
#include "Scheduler_01.h"
#elif SCHEDULERNO == 2
#include "Scheduler_02.h"
#elif SCHEDULERNO == 3
#include "Scheduler_03.h"
#elif SCHEDULERNO == 4
#include "Scheduler_04.h"
#endif


#endif /* SRC_RUNTIME_SCHEDULERS_SCHEDULER_H_ */
