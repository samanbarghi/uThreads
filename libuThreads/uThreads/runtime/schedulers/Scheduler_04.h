/*
 * Scheduler_02.h
 *
 *  Created on: May 4, 2016
 *      Author: Saman Barghi
 */

#ifndef SRC_RUNTIME_SCHEDULERS_SCHEDULER_02_H_
#define SRC_RUNTIME_SCHEDULERS_SCHEDULER_02_H_
#include "../../generic/Semaphore.h"
#include "../kThread.h"
#include "../../io/IOHandler.h"

class uThread;

#define NPOLLBULKPUSH

template<typename T>
class NIBlockingMPSCQueue {
public:
    class Node {
        friend class NIBlockingMPSCQueue<T>;
        Node* volatile next;
        T*  state;
      public:
        constexpr Node(T* ut) : next(nullptr), state(ut) {}
        void setState(T* ut){ state = ut;};
        T* getState(){return state;};

    } __packed;
private:
    Node* volatile      head;
    Node                stub;

    std::atomic<Node*>  tail;

    bool insert(Node& first, Node& last){
      last.Node::next = nullptr;
      Node* prev = tail.exchange(&last, std::memory_order_acquire);
      bool was_empty = ((uintptr_t)prev & 1) != 0;
      prev = (Node*)((uintptr_t)prev & ~1);
      prev->Node::next = &first;
      return was_empty;
    }

public:
    NIBlockingMPSCQueue(): stub(nullptr), tail( (Node*) ((uintptr_t)&stub | 1 )),head(&stub){}

    //push a single element
    bool push(Node& elem){return insert(elem, elem);}
    //Push multiple elements in a form of a linked list, linked by next
    bool push(Node& first, Node& last){return insert(first, last);}

    // pop operates in chunk of elements and re-inserts stub after each chunk
    Node* pop(){
        Node* hhead = head;
l_retry:
        Node* next = head->next;
        if(next){
            head = next;
            hhead->state = next->state;
            return hhead;
        }
        Node* ttail = tail;
        if(((uintptr_t)ttail & 1) != 0) return nullptr;

        if(head != ttail){                     // current chunk empty
            // wait for producer in insert()
            while(head->next == nullptr) asm volatile("pause");
            goto l_retry;
        }else{
            //If tail is marked empty, queue should not have been scheduled
            Node* expected = head;
            Node* xchg = (Node*)((uintptr_t)expected | 1);
            if(tail.compare_exchange_strong(expected, xchg, std::memory_order_release)){
                return nullptr; //The Queue is empty
            }//otherwise another thread is inserting
            goto l_retry;
        }
    }
};

struct UTVar{
    NIBlockingMPSCQueue<uThread>::Node*   node;

    UTVar(){
        node = new NIBlockingMPSCQueue<uThread>::Node(nullptr);
    }

};
/*
 * Local kThread objects related to the
 * scheduler. will be instantiated by static __thread
 */
struct KTLocal{};
/*
 * Per kThread variable related to the scheduler
 */
struct KTVar{
};

struct ClusterVar{};

class Scheduler {
    friend class kThread;
    friend class Cluster;
    friend class IOHandler;
    friend class uThread;
private:

    semaphore sem;
    NIBlockingMPSCQueue<uThread> runQueue;
    NIBlockingMPSCQueue<uThread>::Node* tmpNode = nullptr;

    /* ************** Scheduling wrappers *************/
    //Schedule a uThread on a cluster
    static void schedule(uThread* ut, kThread& kt){
        assert(ut != nullptr);
        auto node =  ut->utvar->node;
        assert(node != nullptr);
        node->setState(ut);
        ut->utvar->node = nullptr;

        if(kt.scheduler->runQueue.push(*node))
            kt.scheduler->sem.post();
    }
    //Put uThread in the ready queue to be picked up by related kThreads
    void schedule(uThread* ut) {
        assert(ut != nullptr);
        auto node =  ut->utvar->node;
        assert(node != nullptr);
        node->setState(ut);
        ut->utvar->node = nullptr;
        if(runQueue.push(*node))
            sem.post();
    }

    //Schedule many uThreads
    void schedule(IntrusiveQueue<uThread>& queue, size_t count) {
        std::cerr << "BulkPush is not supported with this scheduler" << std::endl;
        exit(EXIT_FAILURE);
    }

    uThread* nonBlockingSwitch(kThread& kt){
        IOHandler::iohandler.nonblockingPoll();
        uThread* ut = nullptr;
        auto node = runQueue.pop();

        if(node == nullptr){
            if ( kt.currentUT->state == uThread::State::YIELD)
                return nullptr; //if the running uThread yielded, continue running it
            ut = kt.mainUT;
        }else{
            ut = node->getState();
            if(tmpNode != nullptr){
                ut->utvar->node = tmpNode;
            }else{
                ut->utvar->node = new  NIBlockingMPSCQueue<uThread>::Node(ut);
            }
            tmpNode = node;
        }
        assert(ut != nullptr);
        return ut;
    }

    uThread* blockingSwitch(kThread& kt){
        /* before blocking inform the poller thread of our
         * intent.
         */
        IOHandler::iohandler.sem.post();

        sem.wait();
        auto node = runQueue.pop();

        /*
         * We signaled the poller thread, now it's the time
         * to signal it again that we are unblocked.
         */
        while(!IOHandler::iohandler.sem.trywait());

        uThread* ut = node->getState();
        if(tmpNode != nullptr){
            ut->utvar->node = tmpNode;
        }else{
            ut->utvar->node = new  NIBlockingMPSCQueue<uThread>::Node(ut);
        }
        tmpNode = node;
         assert(ut != nullptr);
        return ut;
    }

    /* assign a scheduler to a kThread */
    static Scheduler* getScheduler(Cluster& cluster){
        return new Scheduler();
    }

    static void prepareBulkPush(uThread* ut){
        std::cerr << "BulkPush is not supported with this scheduler" << std::endl;
        exit(EXIT_FAILURE);
    }
    static void bulkPush(){
        std::cerr << "BulkPush is not supported with this scheduler" << std::endl;
        exit(EXIT_FAILURE);

    }
    static void bulkPush(Cluster &cluster){
        std::cerr << "BulkPush is not supported with this scheduler" << std::endl;
        exit(EXIT_FAILURE);
    }
    static void bulkdPush(kThread &kt){
        std::cerr << "BulkPush is not supported with this scheduler" << std::endl;
        exit(EXIT_FAILURE);
    }
};

#endif /* SRC_RUNTIME_SCHEDULERS_SCHEDULER_02_H_ */
