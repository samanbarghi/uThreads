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

#ifndef UTHREADS_UTHREAD_POOL_H_
#define UTHREADS_UTHREAD_POOL_H_

#include <atomic>
#include <queue>
#include <utility>
#include "BlockingSync.h"
#include "uThread.h"

namespace uThreads {
namespace runtime {
class uThreadPool {
 private:
    // This struct is replaced by the args the first time a uThread is created
    struct Argument {
        funcvoid1_t func;
        void *args;

        uThreadPool *utp;

        Argument(funcvoid1_t func, void *args, uThreadPool *utp) :
                func(func), args(args), utp(utp) {}

    };
    // Total number of uThreads belonging to this thread pool
    std::atomic<unsigned int> totalnumuThreads;
    // idle uThreads
    std::atomic<unsigned int> idleuThreads;

    // Mutex to protect data structures in thread pool
    Mutex mutex;
    // Condition variable to block idle uThreads on
    ConditionVariable cv;

    std::queue<std::pair<funcvoid1_t, void *>> taskList;

    static void run(void *);


 public:
    uThreadPool();

    virtual ~uThreadPool();

    void uThreadExecute(funcvoid1_t, void *, Cluster &);

};  // class uThreadPool
}  // namespace runtime
}  // namespace uThreads
#endif /* UTHREADS_UTHREAD_POOL_H_ */
