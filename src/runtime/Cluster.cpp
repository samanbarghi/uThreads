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

#include <runtime/schedulers/Scheduler.h>
#include <stdlib.h>
#include "Cluster.h"
#include "kThread.h"
#include "io/IOHandler.h"

std::atomic_ushort Cluster::clusterMasterID(0);

std::vector<Cluster*> Cluster::clusterList;

Cluster::Cluster() : numberOfkThreads(0),
        clustervar(new ClusterVar), ktLast(0) {
    //TODO: IO handler should be only applicable for IO Clusters
    //or be created with the first IO call
    initialSynchronization();
}

void Cluster::initialSynchronization() {
    std::lock_guard<std::mutex> lg(Cluster::defaultCluster.mtx);

    //add cluster to the Cluster vector
    clusterList.emplace_back(this);

    if (clusterMasterID + 1 < UINTMAX_MAX)
        clusterID = clusterMasterID++;
    else
        exit(EXIT_FAILURE);
}
Cluster::~Cluster() {
}

void Cluster::addNewkThread(kThread& kt){
    std::lock_guard<std::mutex> lg(mtx);
    ktVector.emplace_back(&kt);

    /*
     * Increase the number of kThreads in the cluster.
     * Since this is always < totalNumberofKTs, it will
     * not overflow.
     */
     numberOfkThreads++;
}

kThread* Cluster::assignkThread(){
    assert(!ktVector.empty());
    size_t next = (ktLast+1)%(ktVector.size());
    size_t kt = ktLast.exchange(next, std::memory_order_relaxed);
    return ktVector[kt];
}
