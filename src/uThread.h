/*
 * uThread.h
 *
 *  Created on: Oct 23, 2014
 *      Author:  Saman Barghi
 */

#ifndef UTHREAD_H_
#define UTHREAD_H_

#define DEFAULT_UTHREAD_PRIORITY 20

typedef unsigned short priority_t;		//Thread priority type

class uThread {
private:
	priority_t priority;				//Threads priority, lower number means higher priority
public:
	uThread();
	virtual ~uThread();

	void setPriority(priority_t);
	priority_t getPriority() const;
};

/*
 * An object for comparing uThreads based on their priorities.
 * This will be used in priority queues to determine which uThread
 * should run next.
 */
class CompareuThread{
public:
	bool operator()(uThread& ut1, uThread& ut2){
		int p1 = ut1.getPriority();
		int p2 = ut2.getPriority();
		if(p1 < p2 || p1 == p2){
			return true;
		}else{
			return false;
		}
	}
};

#endif /* UTHREAD_H_ */
