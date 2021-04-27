#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "dsutil.h"
#include "batch.h"
#include "message.h"

#define MAX_THREADS 16

/*
int batch_run(){
	Threadpool* pool = create_threadpool(args);
    if (pool == NULL) return -1;  
    
    destroy_threadpool(pool);
    free(pool);
    qarray_destroy();
    return 0;
}

Threadpool* create_threadpool(Threadargs* args){
	Threadpool* pool = NULL;

	pool = malloc(sizeof(Threadpool));
	int nqueries = qarray_size();

	pool->nthreads = nqueries < MAX_THREADS ? nqueries : MAX_THREADS;
	pool->threads = (pthread_t*)malloc(pool->nthreads*sizeof(pthread_t));

	pthread_mutex_init(&lock, NULL);
	pool->cur_index = 0;

	for(int i= 0; i < pool->nthreads; i++){
		pthread_create(&(pool->threads[i]), NULL, shared_scan, (void*)(pool));
	}
	return pool;
}

void destroy_threadpool(Threadpool* pool){
	void* status;
	for(int i= 0; i < pool->nthreads; i++){
		pthread_join(pool->threads[i], &status);
	}
	free(pool->threads);
	free(pool->threadids);
	pthread_mutex_destroy(&lock);
}

void* shared_scan(void* p){
	message send_message;
	DbOperator* dbo = NULL;
	Threadpool* pool =  (Threadpool*)p;
	int srvrun = 1;

	while(block = qarray_next(pool)){
		pool->curblock = block;
		while((dbo = qarray_next(pool)) != NULL){
			execute_DbOperator(dbo, &send_message);
			dbo->isbatched = false;
			db_operator_free(dbo, &srvrun);
			free(send_message.payload);
			send_message.payload = NULL;
		}
	}	
	return NULL;
}
*/

/// working set
/*
int batch_run(){
	Threadpool* pool = create_threadpool();
    if (pool == NULL) return -1;  
    
    Threadargs* args = calloc(pool->nthreads, sizeof(Threadargs));
    for(int i= 0; i < pool->nthreads; i++){
    	args[i].id = i;
    	args[i].pool = pool;
		pthread_create(&(pool->threads[i]), NULL, shared_scan, (void*)(&args[i]));
	}

    destroy_threadpool(pool);
    qarray_destroy();
    for(int i= 0; i < pool->nthreads; i++){
    	args[i].pool = NULL;
	}

	free(args);
    free(pool);
    
    return 0;
}

Threadpool* create_threadpool(){
	Threadpool* pool = NULL;

	pool = malloc(sizeof(Threadpool));
	int nqueries = qarray_size();

	pool->nthreads = nqueries < MAX_THREADS ? nqueries : MAX_THREADS;
	pool->threads = (pthread_t*)malloc(pool->nthreads*sizeof(pthread_t));
	pool->nqueries = nqueries;
	pool->cur_index = 0;

	pthread_mutex_init(&lock, NULL);
	return pool;
}

void destroy_threadpool(Threadpool* pool){
	void* status;
	for(int i= 0; i < pool->nthreads; i++){
		pthread_join(pool->threads[i], &status);
	}
	free(pool->threads);
	pthread_mutex_destroy(&lock);
}


void* shared_scan(void* p){
	//message send_message;
	DbOperator* dbo = NULL;
	Threadargs* arg = (Threadargs*)p;


	int count = 0;
	int index = arg->pool->nthreads*count +  arg->id;

	while (index < arg->pool->nqueries){
	    dbo = qarray_at(index);
		if (dbo == NULL) break;
		
		execute_DbOperator(dbo, &send_message);	
		free(send_message.payload);
		send_message.payload = NULL;
	
		count++;
		index = arg->pool->nthreads*count +  arg->id;
	}
		
	return NULL;
}
*/








int batch_run(){
	Threadpool* pool = create_threadpool();
    if (pool == NULL) return -1;  
    
    Threadargs* args = calloc(pool->nthreads, sizeof(Threadargs));
    for(int i= 0; i < pool->nthreads; i++){
    	args[i].id = i;
    	args[i].pool = pool;
		pthread_create(&(pool->threads[i]), NULL, shared_scan, (void*)(&args[i]));
	}

    destroy_threadpool(pool);
    tarray_destroy();
    for(int i= 0; i < pool->nthreads; i++){
    	args[i].pool = NULL;
	}

	free(args);
    free(pool);
    
    return 0;
}

Threadpool* create_threadpool(){
	Threadpool* pool = NULL;

	pool = malloc(sizeof(Threadpool));
	int nqueries = tarray_size();

	pool->nthreads = nqueries < MAX_THREADS ? nqueries : MAX_THREADS;
	pool->threads = (pthread_t*)malloc(pool->nthreads*sizeof(pthread_t));
	pool->nqueries = nqueries;
	pool->cur_index = 0;

	pthread_mutex_init(&lock, NULL);
	return pool;
}

void destroy_threadpool(Threadpool* pool){
	void* status;
	for(int i= 0; i < pool->nthreads; i++){
		pthread_join(pool->threads[i], &status);
	}
	free(pool->threads);
	pthread_mutex_destroy(&lock);
}


void* shared_scan(void* p){
	Task* t = NULL;
	Threadargs* arg = (Threadargs*)p;
	int start = 0;
	int isdone = 0;

	while (true){
		int count = 0;
		int index = arg->pool->nthreads*count +  arg->id;
		while (index < arg->pool->nqueries){
		    t = tarray_at(index);
			if (t == NULL || t->dbo == NULL) break;
			isdone = (t->func)(t->dbo, start, &t->ntuples, t->positions);
			count++;
			index = arg->pool->nthreads*count +  arg->id;
		}
		start = start + VEC_SIZE;
		if (isdone) break;
	}
		
	return NULL;
}