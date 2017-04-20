/*
 * Fibnoacci.cpp
 *
 *  Created on: Jul 11, 2016
 *      Author: Saman Barghi
 */
#include <uThreads/uThreads.h>
#include <iostream>
#include <cstdlib>

using namespace std;


#ifdef _DEBUG
#define DEBUGPRINT(x)  cout << x;
#else
#define DEBUGPRINT(x)
#endif


void task(void* v, void* r){
   int value = *(int*)v;
   int* result = (int*)r;

   DEBUGPRINT( "Fib(" << value << ")" << endl);
   if(value < 2)
       *result = value;
   else{
       int val1 = value -1, val2 = value-2;
       int res1 = 0, res2 = 0;

       uThread* ut1 = uThread::create(true);
       uThread* ut2 = uThread::create(true);

       DEBUGPRINT( "Calling Fib(" << val1 << ")" << endl);
       ut1->start(Cluster::getDefaultCluster(), (void*)task, (void*)&val1, (void*)&res1);
       ut1->join();
       DEBUGPRINT( "Result Fib(" << val1 << ") : " << res1 << endl);
       *result = res1;

       DEBUGPRINT( "Calling Fib(" << val2 << ")" << endl);
       ut2->start(Cluster::getDefaultCluster(), (void*)task, (void*)&val2, (void*)&res2);
       ut2->join();
       DEBUGPRINT( "Result Fib(" << val2 << ") : " << res2 << endl);
       *result += res2;
   }
   DEBUGPRINT( "Exiting Fib("<< value << "):" << *result << endl);
}

int main(int argc, char* argv[]) {

    if(argc != 2){
        cerr << "Error! Please provide a number" << endl;
        exit(EXIT_FAILURE);
    }
    kThread kt(Cluster::getDefaultCluster());
    int fibValue = atoi(argv[1]);
    int finalResult = 0;

    uThread* ut = uThread::create(true);
    ut->start(Cluster::getDefaultCluster(), (void*)task, (void*)&fibValue, (void*)&finalResult);
    ut->join();

    cout << finalResult << endl;
    return 0;
}
