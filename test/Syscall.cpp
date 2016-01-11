#include "uThreads.h"
#include <unistd.h>

static void run(void* args){
	//assume args is an int
	int value = *(int*)args;
	write(0, "This is run\n", 13);

}
int main(){

//	std::cout<<"Start of Main Function"<<std::endl;

	Cluster cluster;
	kThread kt(cluster);

	write(0, "This is run\n", 13);
	puts("This is a test");
	uThread* ut;
	int value[100000];
	for (int i=0; i< 2; i++){
		//Numbers should be written in order
		value[i] = i;
		if(i%2 == 0) ut = uThread::create(cluster, (funcvoid1_t)run, &value[i]);
		else  ut = uThread::create((funcvoid1_t)run, &value[i]);
	}
	while(uThread::getTotalNumberofUTs() > 1){
		uThread::yield();
	}
//	cout << "End of Main Function!" << endl;

	return 0;
}

