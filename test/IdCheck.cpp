#include "uThreads.h"
#include <stdio.h>
#include <iostream>
#include <unistd.h>


using namespace std;
static void voidrun(void* args){

	cout << "uThreadID : " << kThread::currentKT->currentUT->getUthreadId() << " | Total uts: " << kThread::currentKT->currentUT->getTotalNumberofUTs() << endl;
	uThread::terminate();
}

static void run(void* args){
	//assume args is an int
	int value = *(int*)args;

	uThread* ut = uThread::create();
	ut->start(kThread::currentKT->currentUT->getCurrentCluster(), (ptr_t)voidrun, &value);
	cout << "uThreadID : " << kThread::currentKT->currentUT->getUthreadId() << " | Total uts: " << kThread::currentKT->currentUT->getTotalNumberofUTs() << endl;
	uThread::terminate();
}
int main(){

	std::cout<<"Start of Main Function"<<std::endl;

	Cluster cluster;
	cout << "First Cluster ID: " << cluster.getClusterID() << endl;
	Cluster cluster2;
	cout << "Second Cluster ID: " << cluster2.getClusterID() << endl;

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

