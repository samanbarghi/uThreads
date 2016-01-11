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
		value[i] = i;
		if(i%2 == 0)	ut = uThread::create((funcvoid1_t)run, &value[i], cluster);
		else	ut = uThread::create((funcvoid1_t)run, &value[i]);
	}

	while(uThread::getTotalNumberofUTs() > 1){
			uThread::yield();
		}
	cout << "End of Main Function!" << endl;

	return 0;
}

