/*
 * kThread.cpp
 *
 *  Created on: Oct 27, 2014
 *      Author: Saman Barghi
 */

#include "kThread.h"
#include <iostream>

using namespace std;
kThread::kThread() : threadSelf(&kThread::run, this) {
	// TODO Auto-generated constructor stub

}


kThread::~kThread() {
	threadSelf.join();						//wait for the thread to terminate properly
}

void kThread::run() {
	cout << "Started Running!" << endl;
}

