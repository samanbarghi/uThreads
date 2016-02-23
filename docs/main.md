uThreads: Concurrent User-level Threads in C++             {#mainpage}
============
uThreads is a concurrent library based on cooperative scheduling of user-level threads implemented in C++. User-level threads are lightweight threads that execute on top of kernel threads to provide concurrency as well as parallelism. Kernel threads are necessary to utilize processors, but they come with the following drawbacks: 
* Each suspend/resume operation involves a kernel context switch
* Thread preemption causes additional overhead
* Thread priorities and advanced scheduling causes additional overhead

Cooperative user-level threads, on the other hand, provide light weight context switches and omit the additional overhead of preemption and kernel scheduling. Most Operating Systems only support a 1:1 thread mapping (1 user-level thread to 1 kernel-level thread), where multiple kernel threads execute at the same time to utilize multiple cores and provide parallelism. e.g., Linux supports only 1:1 thread mapping. There is also N:1 thread mapping, where multiple user-level threads can be mapped to a single kernel-level thread. The kernel thread is not aware of the user-level threads existence. With N:1 mapping if the application blocks at the kernel level, it means blocking all user-level threads and application stops executing. This problem can be solved by using multiple kernel-level threads and map multiple user-level threads to each of them. Thus, creating the third scenario with M:N or hybrid mapping. e.g., [uC++](https://plg.uwaterloo.ca/usystem/uC++.html) supports M:N mapping.

uThreads supports M:N mapping of *uThreads* (user-level threads) over *kThreads* (kernel-level threads) with cooperative scheduling. kThreads can be grouped together by *Clusters*, and uThreads can migrate among Clusters. Figure 1 shows the structure of an application implemented using uThreads. Each part is explained further in the following. 

![Figure 1: uThreads Architecture](architecture.png)

**Clusters** are used to group kThreads together. Each Cluster can contain one or more kThreads, but each kThread only belongs to a single Cluster. Each Cluster includes a single *ReadyQueue* which is used to schedule uThreads over kThreads in that Cluster. Application programmer decides how many kThreads belong to a ReadyQueue by assigning them to the related Cluster. 

**kThreads** are kernel-level threads (std::thread), that are the main vehicle to utilize cores and execute the program. Each kThread can only pull uThreads from the ReadyQueue of the Cluster it belongs to, but it can push uThreads to the ReadyQueue of any Cluster in the application. The former can happen when uThreads *yield* or *block* at user level, and the latter happens when uThreads *migrate* to another Cluster. Migration let the code execute on a different set of kThreads based on the requirements of the code. 

**uThreads** are the main building blocks of the library. They are either sitting in the ReadyQueue waiting to be picked by a kThread, running by a kThread, or blocked and waiting for an event to occur. uThreads are being scheduled cooperatively over Clusters, they can either yield, migrate or block on an event to let other uThreads utilized the same kThread they are being executed over. 

Each application has at least one Cluster, one kThread and one uThread. Each C++ application has at least one thread of execution (kernel thread) which runs the *main()* function. A C++ application that is linked with uThreads library, upon execution, creates a *defaultCluster*, a wrapper around the main execution thread and call it *defaultkThread*, and also  a uThread called *mainUT* to take over the defaultkThread stack and run the *main* function. 

In addition, each Cluster by default has a *Poller kThread* which is responsible for polling the network devices, and multiplexing network events over the Cluster. uThreads provide a user-level blocking network events, where network calls are non-blocking at the kernel-level but uThreads block on network events if the device is not ready for read/write. The poller thread is thus responsible for unblock the uThreads upon receiving the related network event. The poller thread is using *edge triggered epoll* in Linux, and the model is similar to [Golang](https://golang.org/). 

By default there is a uThread cache to cache uThreads that finished executing and avoid the extra overhead of memory allocation. Currently, this cache only supports uThreads with same stack size and does not support the scenario where stack sizes are different. This feature will be added in the near future.

Migration and Joinable uThreads
-------------------------------

uThreads can be joinable, where upon creating the creator has to wait for them to finish execution and join with them. So there are two ways to execute a piece of code on another Cluster: 
* **Migration:** uThread can migrate to another Cluster to execute a  piece of code and it can either migrate back to the previous Cluster or continue the execution on the same Cluster or migrate to a different Cluster. The following code demonstrates a simple scenario to migrate to a different cluster and back, assuming uThread is executing on the *defaultCluster*: 
~~~~~~~~~~~~~~~~~{.cpp}
Cluster *cluster1;

void func(){
	// some code 
	migrate(*cluster1);
	// code to run on cluster1
	migrate(Cluster::getDefaultCluster());
 // some more code 
}


int main(){
	
	cluster1 = new Cluster();
	kThread kt(*cluster1);
	uThread *ut = uThread::create(); 
	ut->start(Cluster::getDefaultCluster(), func);
.
.
.
}
~~~~~~~~~~~~~~~~~

* **Joinable thread:** Create a joinable thread on the remote Cluster and wait for it to finish execution. While waiting, the uThread is blocked at user-level and will be unblocked by the newly created uThread.
~~~~~~~~~~~~~~~~~{.cpp}
Cluster *cluster1;

void run(){
	//code to run on cluster1
}
void func(){
	// some code 
	uThread *ut2 = uThread::create(true); //create a joinable thread
	ut2->start(cluster1, run);
	ut2->join(); //wait for ut2 to finish execution and join
 // some more code 
}


int main(){
	
	cluster1 = new Cluster();
	kThread kt(*cluster1);
	uThread *ut = uThread::create(); 
	ut->start(Cluster::getDefaultCluster(), func);
.
.
.
}
~~~~~~~~~~~~~~~~~ 

User-level Blocking Synchronization Primitives
------------------------------------
uThreads also provides user-level blocking synchronization and mutex primitives. It has basic Mutex, Condition Variable and Semaphore. You can find examples of their usage under *test* directory in the [github repo](https://github.com/samanbarghi/uThreads).


Examples
----------------------------------
You can find various examples under the test directory in the [github repo](https://github.com/samanbarghi/uThreads). There is an [EchoClient](https://github.com/samanbarghi/uThreads/blob/master/test/EchoClient.cpp) and [EchoServer](https://github.com/samanbarghi/uThreads/blob/master/test/EchoServer.cpp) implemented using uThreads. 

There is also a simple [webserver](https://github.com/samanbarghi/uThreads/blob/master/test/webserver.cpp) to test uThreads functionality. 

For performance comparisons, memached code has been updated to use uThreads instead of event loops (except the thread that accepts connections), where tasks are assigned to uThreads instead of using the underlying event library. The code can be found [here](https://github.com/samanbarghi/memcached/tree/uThreads).

