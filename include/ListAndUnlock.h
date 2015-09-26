/*
 * ListAndUnlock.h
 *
 *  Created on: Aug 15, 2015
 *      Author: saman
 */

#ifndef INCLUDE_LISTANDUNLOCK_H_
#define INCLUDE_LISTANDUNLOCK_H_


#include <mutex>
#include <unordered_map>
#include <vector>
#include <utility>
#include <assert.h>
#include <iostream>
#include "EmbeddedList.h"

class Mutex;
class uThread;

/*
 * The purpose of this class is that given a data structure,
 * and a lock, it pushes the current running uThread to the structure
 * and then unlock the lock. This is used for post-switch function in kThread
 */

class ListAndUnlock {
	virtual void _PushAndUnlock() = 0;
protected:
	uThread* ut;
	ListAndUnlock();
public:
	void PushAndUnlock(){_PushAndUnlock();};
	virtual ~ListAndUnlock(){}; //nothing has been allocated
};

/*
 * EmbeddedList and Mutex or std::mutex
 */

class EmbeddedListAndUnlock : public ListAndUnlock{

	EmbeddedList<uThread>* list;
	Mutex* uMutex = nullptr;
	std::mutex* mutex = nullptr;
	void _PushAndUnlock();

public:
	EmbeddedListAndUnlock(EmbeddedList<uThread>* list, Mutex* uMutex) : ListAndUnlock(), list(list), uMutex(uMutex){};
	EmbeddedListAndUnlock(EmbeddedList<uThread>* list, std::mutex* mutex) : ListAndUnlock(), list(list), mutex(mutex){};
	virtual ~EmbeddedListAndUnlock(){}; //nothing has been allocated
};


template <typename T, typename T2> class MapAndUnlock : public ListAndUnlock{

	std::unordered_map<T, T2*>* map;
	std::mutex* mutex  = nullptr;
	T id = -1;
	T2* data = nullptr;
	void _PushAndUnlock(){
	    assert(map != nullptr);
	    assert(data != nullptr);
	    assert(id != -1);

	    if(map->find(id) == map->end()) //only add if it does not exists already
	        map->insert(std::make_pair(this->id, data));

		if(mutex != nullptr) mutex->unlock();
	}

public:
	MapAndUnlock(std::unordered_map<T, T2*>* map, T id, T2* data, std::mutex*  mutex) : map(map), id(id), data(data), mutex(mutex){};
	~MapAndUnlock(){};
};


#endif /* INCLUDE_LISTANDUNLOCK_H_ */
