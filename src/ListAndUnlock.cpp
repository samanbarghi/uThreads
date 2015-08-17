/*
 * ListAndUnlock.cpp
 *
 *  Created on: Aug 15, 2015
 *      Author: saman
 */
#include "ListAndUnlock.h"
#include "kThread.h"
#include "BlockingSync.h"

ListAndUnlock::ListAndUnlock() : ut(kThread::currentKT->currentUT){}; //since only used in post-switch function in kThread, just allow current uThread to be pushed

void EmbeddedListAndUnlock::_PushAndUnlock(){
	list->push_back(this->ut);

	//Release the mutex
	if(mutex) mutex->unlock();
	else if(uMutex) uMutex->release();
};
