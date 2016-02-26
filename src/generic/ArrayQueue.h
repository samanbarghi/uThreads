/*
 * ArrayQueue.h
 *
 *  Created on: Dec 16, 2015
 *      Author: Saman Barghi
 */

#ifndef INCLUDE_ARRAYQUEUE_H_
#define INCLUDE_ARRAYQUEUE_H_

#include <array>
#include <assert.h>

// Creating a class named Queue.
template <typename T, size_t SIZE>
class ArrayQueue
{
private:
    T* A[SIZE];
    int ifront, irear, counter;
public:
    ArrayQueue(): ifront(-1), irear(-1), counter(0){};
    ~ArrayQueue(){}
    bool empty(){  return (ifront == -1 && irear == -1);}
    bool full(){return (irear+1)%SIZE== ifront ? true : false;}
    // Inserts an element in queue at rear end
    bool push(T* t)
    {
        if(full()) return false;
        if (empty()){ifront = irear = 0;}
        else irear = (irear+1)%SIZE;
        A[irear] = std::move(t);
        counter++;
        return true;
    }
    bool pop()
    {
        if(empty()) return false;
        else if(ifront == irear ) irear = ifront = -1;
        else ifront = (ifront+1)%SIZE;
        counter--;
        return true;
    }
    T* front()
    {
        assert(ifront != -1);
        return A[ifront];
    }
    T*  (&begin())[SIZE]{
        return A;
    }
    //If did a bulk enqueue using the iterator
    void updateIndexes(size_t count){
        //array should have been originally empty before the bulk insert
        assert(empty());
        assert(count <= SIZE);

        //if nothing was added ignore it
        if(count == 0) return;
        ifront = 0;
        irear = count-1%SIZE;
        counter = count;
    }
    size_t size(){
        return SIZE;
    }
    void reset(){
        ifront = irear = -1;
        counter =0;
    }
    int count(){
        return counter;
    }
};



#endif /* INCLUDE_ARRAYQUEUE_H_ */
