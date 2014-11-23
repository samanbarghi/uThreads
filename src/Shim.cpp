/*
 * Shim.cpp
 *
 *  Created on: Nov 18, 2014
 *      Author: Saman Barghi
 */
#define _GNU_SOURCE
#include "Cluster.h"
#include "kThread.h"
#include <dlfcn.h>
#include <stdio.h>
#include <iostream>

static  size_t  (*real_fread)   (void *ptr, size_t size, size_t nmemb, FILE *stream) = (size_t(*)(void*, size_t, size_t, FILE*))dlsym(RTLD_NEXT, "fread");
static  ssize_t (*real_read)    (int fd, void *buf, size_t count) = NULL;
static  ssize_t (*real_write)   (int fd, const void *buf, size_t count) = NULL;
static  FILE*   (*real_fopen)   (const char *filename, const char *mode) = (FILE* (*)(const char*, const char*))dlsym(RTLD_NEXT, "fopen");;
static  int     (*real_fclose)  (FILE *fp) =  (int (*)(FILE*))dlsym(RTLD_NEXT, "fclose");
static  int     (*real_open)    (const char *pathname, int flags) = (int (*)(const char*, int))dlsym(RTLD_NEXT, "open");
static  int     (*real_puts)    (const char* str) = NULL;

static  int     (*real_fseek)   (FILE *stream, long offset, int whence) = (int (*)(FILE*, long, int))dlsym(RTLD_NEXT, "fseek");
static  ssize_t (*real_send)    (int s, const void *buf, size_t len, int flags) = (ssize_t (*)(int, const void*, size_t, int))dlsym(RTLD_NEXT, "send");
static  ssize_t (*real_recv)    (int sockfd, void *buf, size_t len, int flags)  = (ssize_t (*)(int, void*, size_t, int))dlsym(RTLD_NEXT, "recv");
static  void*   (*real_malloc)  (size_t size) = (void* (*)(size_t))dlsym(RTLD_NEXT, "malloc");
static  void    (*real_free)    (void *ptr) = (void (*)(void*))dlsym(RTLD_NEXT, "free");
static  int     (*real_close)   (int fd) = (int (*)(int))dlsym(RTLD_NEXT, "close");
static  void    (*real_rewind)  (FILE *stream) = (void (*)(FILE*))dlsym(RTLD_NEXT, "rewind");



int puts(const char* str){
	Cluster* tmpCluster = (Cluster*)kThread::currentKT->currentUT->getCurrentCluster();
	kThread::currentKT->currentUT->migrate(&Cluster::syscallCluster);

	real_puts = (int (*)(const char*)) dlsym(RTLD_NEXT, "puts");

	int ret = real_puts(str);

	kThread::currentKT->currentUT->migrate(tmpCluster);
	return ret;
}

ssize_t write (int fd, const void * buf, size_t n){
	std::cout << "This is Write" << std::endl;
	//real_write = dlsym(RTLD_NEXT, "write");
	//return real_write(fd, buf, count);
	return 0;

}
size_t freadx(void *ptr, size_t size, size_t nmemb, FILE *stream){
	Cluster* tmpCluster = (Cluster*)kThread::currentKT->currentUT->getCurrentCluster();
	kThread::currentKT->currentUT->migrate(&Cluster::syscallCluster);

//	real_fread = (size_t(*)(void*, size_t, size_t, FILE))dlsym(RTLD_NEXT, "fread");

	size_t ret = real_fread(ptr, size, nmemb, stream);
	kThread::currentKT->currentUT->migrate(tmpCluster);

	return ret;
}
ssize_t read(int fd, void *buf, size_t count){
	std::cout << "This is read" << std::endl;

	real_read = (ssize_t(*)(int, void*, size_t))dlsym(RTLD_NEXT, "read");
	return real_read(fd, buf, count);
}
int fclose(FILE* fp){
	Cluster* tmpCluster = (Cluster*)kThread::currentKT->currentUT->getCurrentCluster();
	kThread::currentKT->currentUT->migrate(&Cluster::syscallCluster);

	int ret = real_fclose(fp);

	kThread::currentKT->currentUT->migrate(tmpCluster);
	return ret;
}
FILE *fopenx(const char *filename, const char *mode){
//	std::cout << "This is fopen beginning" << std::endl;
	Cluster* tmpCluster = (Cluster*)kThread::currentKT->currentUT->getCurrentCluster();
	kThread::currentKT->currentUT->migrate(&Cluster::syscallCluster);

//	std::cout << "This is fopen after migration in Cluster:" << kThread::currentKT->currentUT->getCurrentCluster()->clusterID << std::endl;
//	real_fopen = (FILE* (*)(const char*, const char*))dlsym(RTLD_NEXT, "fopen");
	FILE* ret = real_fopen(filename, mode);

	kThread::currentKT->currentUT->migrate(tmpCluster);

//	std::cout << "This is fopen coming back" << std::endl;
	return ret;
}
int open (const char *pathname, int flags){
	std::cout << "This is Open" << std::endl;

	real_open = dlsym(RTLD_NEXT, "open");
	return real_open(pathname, flags);
}

int fseek(FILE *stream, long offset, int whence){
    Cluster* tmpCluster = (Cluster*)kThread::currentKT->currentUT->getCurrentCluster();
    kThread::currentKT->currentUT->migrate(&Cluster::syscallCluster);
    int ret = real_fseek(stream, offset, whence);
    kThread::currentKT->currentUT->migrate(tmpCluster);
    return ret;
}

ssize_t send(int s, const void *buf, size_t len, int flags){
    Cluster* tmpCluster = (Cluster*)kThread::currentKT->currentUT->getCurrentCluster();
    kThread::currentKT->currentUT->migrate(&Cluster::syscallCluster);
    ssize_t ret = real_send(s, buf, len, flags);
    kThread::currentKT->currentUT->migrate(tmpCluster);
    return ret;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags){
    Cluster* tmpCluster = (Cluster*)kThread::currentKT->currentUT->getCurrentCluster();
    kThread::currentKT->currentUT->migrate(&Cluster::syscallCluster);
    ssize_t ret = real_recv(sockfd, buf, len, flags);
    kThread::currentKT->currentUT->migrate(tmpCluster);
    return ret;
}

//void *malloc(size_t size){
//    Cluster* tmpCluster = (Cluster*)kThread::currentKT->currentUT->getCurrentCluster();
//    kThread::currentKT->currentUT->migrate(&Cluster::syscallCluster);
//    void* ret = real_malloc(size);
//    kThread::currentKT->currentUT->migrate(tmpCluster);
//    return ret;
//}

//void free(void *ptr){
//    Cluster* tmpCluster = (Cluster*)kThread::currentKT->currentUT->getCurrentCluster();
//    kThread::currentKT->currentUT->migrate(&Cluster::syscallCluster);
//    real_free(ptr);
//    kThread::currentKT->currentUT->migrate(tmpCluster);
//}

int close(int fd){
    Cluster* tmpCluster = (Cluster*)kThread::currentKT->currentUT->getCurrentCluster();
    kThread::currentKT->currentUT->migrate(&Cluster::syscallCluster);
    int ret = real_close(fd);
    kThread::currentKT->currentUT->migrate(tmpCluster);
    return ret;
}

void rewind(FILE *stream){
    Cluster* tmpCluster = (Cluster*)kThread::currentKT->currentUT->getCurrentCluster();
    kThread::currentKT->currentUT->migrate(&Cluster::syscallCluster);
    real_rewind(stream);
    kThread::currentKT->currentUT->migrate(tmpCluster);
}
