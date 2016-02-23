/*
 * Joinable.cpp
 *
 *  Created on: Feb 6, 2016
 *      Author: saman
 */
#include "uThreads.h"
#include <iostream>
#include <sstream>

using namespace std;

void run(void* args){
    int* value = (int*)args;

    (*value)++;
    cout << "Inside Joinable: "<< *value << endl;
    (*value)++;
}

int main(){

    Cluster cluster;
    kThread kt(cluster);

    uThread* ut;
    int value[100];

    for (int i=0; i< 100; i++){
        //Numbers should be written in order
        ut = uThread::create(true);
        cout << "ID: " << ut->getID() << endl;
        value[i] = i;
        cout << "Before Joinable: "<< value[i] << endl;

        ut->start(cluster, (ptr_t)run, &value[i]);

        if(!ut->join()){
            cout << "Join Error !" << endl << endl;
            continue;
        }
        cout  << "After Joinable :" << value[i]<< endl << endl;
    }
    while(uThread::getTotalNumberofUTs() > 2){
        uThread::yield();
    }
    cout << "End of Main Function!" << endl;
    return 0;
}


