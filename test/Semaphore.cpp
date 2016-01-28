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
#include "uThreads.h"
#include <stdio.h>
#include <iostream>
#include <unistd.h>

static Mutex mtx;
static Semaphore s(1);
static int counter = 0;

using namespace std;
static void run(void* args){
	//assume args is an int
	int value = *(int*)args;

	s.P();
	cout << "This is run #" <<  value << " - counter #" << counter++ << endl;
	s.V();
}
int main(){

	std::cout<<"Start of Main Function"<<std::endl;

	Cluster cluster;
	kThread kt(cluster);
	kThread kt2(cluster);
	kThread kt1(cluster);
	kThread kt3(cluster);

	uThread* ut;
	int value[10000];
	for (int i=0; i< 10000; i++){
		//Numbers should be written in order
	    ut = uThread::create();
		value[i] = i;
		if(i%2 == 0)    ut->start(cluster, (ptr_t)run, &value[i]);
		else    ut->start(Cluster::getDefaultCluster(), (ptr_t)run, &value[i]);
	}

	while(uThread::getTotalNumberofUTs() > 1){
			uThread::yield();
		}
	cout << "End of Main Function!" << endl;

	return 0;
}

