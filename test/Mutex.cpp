#include "uThreads.h"
#include <stdio.h>
#include <iostream>
#include <unistd.h>

static Mutex mtx;
static mword counter = 0;

using namespace std;
static void run(void* args){
	//assume args is an int
	int value = *(int*)args;

	mtx.acquire();
	cout << "This is run #" <<  value << " - counter #" << counter++ << endl;
	mtx.release();
	uThread::terminate();
}
int main(){

	std::cout<<"Start of Main Function"<<std::endl;

	Cluster cluster;
	kThread kt(cluster);
	kThread kt1(cluster);
	kThread kt2(cluster);
	kThread kt3(cluster);
	uThread* ut;
	int value[100000];
	for (int i=0; i< 100000; i++){
		//Numbers should be written in order
	    ut = uThread::create();
		value[i] = i;
		if(i%2 == 0) ut->start(cluster, (ptr_t)run, &value[i]);
		else  ut->start((ptr_t)run, &value[i]);
	}

	while(uThread::getTotalNumberofUTs() > 2){
		uThread::yield();
	}
	cout << "End of Main Function!" << endl;

	exit(EXIT_SUCCESS);
	return 0;

}

