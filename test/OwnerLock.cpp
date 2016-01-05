#include "uThreads.h"
#include <stdio.h>
#include <iostream>
#include <unistd.h>

static OwnerLock mtx;
static mword counter = 0;

using namespace std;
static void run(void* args){
	//assume args is an int
	int value = *(int*)args;

	mtx.acquire();
	cout << kThread::currentKT->currentUT->getCurrentCluster()->getClusterID() << ":uThreadID: " << kThread::currentKT->currentUT->getUthreadId() << ": This is run #" <<  value << " - counter #" << counter++ << endl;
	kThread::currentKT->printThreadId();
	mtx.release();
}
int main(){

	std::cout<<"Start of Main Function"<<std::endl;

	Cluster* cluster =  new Cluster();
	kThread kt(cluster);
	kThread kt1(cluster);
	kThread kt2(cluster);
	kThread kt3(cluster);
	uThread* ut;
	int value[100000];
	for (int i=0; i< 100000; i++){
		//Numbers should be written in order
		value[i] = i;
		ut = uThread::create((funcvoid1_t)run, &value[i], cluster);
	}
	while(uThread::getTotalNumberofUTs() > 1){
		uThread::yield();
	}
	cout << "End of Main Function!" << endl;

	return 0;
}

