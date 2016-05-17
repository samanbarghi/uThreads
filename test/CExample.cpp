#include "cwrapper.h"
#include <stdio.h>



static void run(void* args){
    int value = *(int*)args;
    uint64_t id = uThread_get_id(uThread_get_current());
    printf("ID: %" PRIu64 ", value: %d\n", id, value);

}

int main(){

    //create a Cluster
    WCluster* cluster = cluster_create();
    //create a kThread over the cluster
    WkThread* kt = kThread_create(cluster);

    WuThread* ut;
    int value[100000];
    for (int i=0; i< 100000; i++){
        //create a uThread
        ut = uThread_create(false);
        value[i] = i;
        //Start the uThread over the prvoided cluster
        if(i%2 == 0) uThread_start(ut, cluster, (void*)run, &value[i], 0, 0);
        else  uThread_start(ut, cluster_get_default(), (void*)run, &value[i], 0, 0);
    }

    while(uThread_get_total_number_of_uThreads() > 2)
        uThread_yield();
    return 0;
}


