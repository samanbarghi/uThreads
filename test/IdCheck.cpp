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


using namespace std;
static void voidrun(void* args){

	cout << "uThreadID : " << uThread::currentUThread()->getID() << " | Total uts: " << uThread::currentUThread()->getTotalNumberofUTs() << endl;
	uThread::terminate();
}

static void run(void* args){
	//assume args is an int
	int value = *(int*)args;

	uThread* ut = uThread::create();
	ut->start(uThread::currentUThread()->getCurrentCluster(), (ptr_t)voidrun, &value);
	cout << "uThreadID : " << uThread::currentUThread()->getID() << " | Total uts: " << uThread::currentUThread()->getTotalNumberofUTs() << endl;
	uThread::terminate();
}
int main(){

	std::cout<<"Start of Main Function"<<std::endl;

	Cluster cluster;
	cout << "First Cluster ID: " << cluster.getID() << endl;
	Cluster cluster2;
	cout << "Second Cluster ID: " << cluster2.getID() << endl;

	kThread kt(cluster);
	kThread kt1(cluster);
	kThread kt2(cluster);
	kThread kt3(cluster);


	uThread* ut;
	int value[100000];
	for (int i=0; i< 100000; i++){
	    ut = uThread::create();
		//Numbers should be written in order
		value[i] = i;
		ut->start(cluster, (ptr_t)run, &value[i]);
	}

	while(uThread::getTotalNumberofUTs() > 2){
		uThread::yield();
	}
	cout << "End of Main Function!" << endl;

	exit(EXIT_SUCCESS);
	return 0;

}

