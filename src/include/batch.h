#ifndef BATCH_H
#define BATCH_H

#include "pthread.h"

pthread_mutex_t lock;

typedef struct Threadpool{
	pthread_t* threads;
	int nthreads;
	int nqueries;
	int cur_index;
}Threadpool;

typedef struct Threadargs{
	Threadpool* pool;
	int id;
}Threadargs;

int batch_run();
void* shared_scan(void* pool);
void destroy_threadpool();
Threadpool* create_threadpool();


#endif