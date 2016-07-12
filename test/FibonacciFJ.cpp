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

class Task{
public:
    int n;
    int result;
    uThread* ut = nullptr;

    Task(int num): n(num),result(0){};

    static void compute(void* t){
        Task* task = (Task*)t;
        if(task->n < 2){
            task->result = task->n;
            return;
        }

        Task t1(task->n-1);
        Task t2(task->n-2);

        t1.fork();
        Task::compute((void*)&t2);
        task->result = t2.result + t1.join();
    }

    void fork(){
        ut = uThread::create(true);
        ut->start(Cluster::getDefaultCluster(), (void*)Task::compute, (void*)this);
    }

    int join(){
        assert(ut != nullptr);
        cout << ut->getID() << endl;
        ut->join();
        return this->result;
    }

};

int main(int argc, char* argv[]) {

    if(argc != 2){
        cerr << "Error! Please provide a number" << endl;
        exit(EXIT_FAILURE);
    }
    //kThread kt(Cluster::getDefaultCluster());
    int fibValue = atoi(argv[1]);

    Task init(fibValue);
    init.fork();

    cout << init.join() << endl;
    return 0;
}
