/*
 * MessageQueueSpike.cpp
 *
 *  Created on: Jan 21, 2015
 *      Author: Saman Barghi
 */

#include "uThread.h"
#include "Cluster.h"
#include "kThread.h"
#include  "MessageQueue.h"
#include "Buffers.h"
#include <stdio.h>
#include <iostream>
#include <unistd.h>

MessageQueue<FixedRingBuffer<int, 10>> mq;

using namespace std;
static void produce(void* args){
    //assume args is an int
    int value = *(int*)args;
    mq.send(value);
    cout << "Producer sent: " << value << endl;
    kThread::currentKT->printThreadId();
    uThread::uexit();
}
static void consume(void* args){
    int result = 0;
    mq.recv(result);
    cout << "Consumer got: " << result << endl;
    kThread::currentKT->printThreadId();
    uThread::uexit();
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
        if(i%2 == 0) ut = uThread::create((funcvoid1_t)produce, &value[i], cluster);
        else  ut = uThread::create((funcvoid1_t)consume, &value[i]);
    }
    while(uThread::getTotalNumberofUTs() > 1){
        uThread::yield();
    }
    cout << "End of Main Function!" << endl;

    exit(1);
    return 0;

}

