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

#include <stdlib.h>
#include "Cluster.h"
#include "kThread.h"
#include "ReadyQueue.h"
#include "io/IOHandler.h"

std::atomic_ushort Cluster::clusterMasterID(0);

Cluster::Cluster() :
        numberOfkThreads(0) {
    //TODO: IO handler should be only applicable for IO Clusters
    //or be created with the first IO call
    readyQueue = new ReadyQueue();
    initialSynchronization();
    iohandler = IOHandler::create(*this);
}

void Cluster::initialSynchronization() {
    if (clusterMasterID + 1 < UINTMAX_MAX)
        clusterID = clusterMasterID++;
    else
        exit(EXIT_FAILURE);
}

Cluster::~Cluster() {
}


void Cluster::schedule(uThread* ut) {
    assert(ut != nullptr);
    //Scheduling uThread
    readyQueue->push(ut);
}

void Cluster::scheduleMany(IntrusiveList<uThread>& queue, size_t count) {
    assert(!queue.empty());
    readyQueue->pushMany(queue, count);
}

uThread* Cluster::tryGetWork() {
    return readyQueue->tryPop();
}

/*
 * iPull multiple uThreads from the ready queue and
 * push into the kThread local queue. This is nonblocking.
 */
ssize_t Cluster::tryGetWorks(IntrusiveList<uThread> &queue) {
    return readyQueue->tryPopMany(queue);
}

ssize_t Cluster::getWork(IntrusiveList<uThread> &queue) {
    return readyQueue->popMany(queue);
}
