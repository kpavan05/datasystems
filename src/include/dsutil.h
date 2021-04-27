#ifndef __DSUTIL_H__
#define __DSUTIL_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cs165_api.h"
#include "batch.h"

typedef struct queue{
	DbOperator *dbo;
	struct queue *next;
}queue;

typedef int (*sharedFunc)(DbOperator* ,unsigned int, unsigned int*, unsigned int*);

typedef struct Task{
	DbOperator* dbo;
	sharedFunc func;
	unsigned int ntuples;
	unsigned int nallocsz;
	unsigned int* positions;
}Task;

typedef struct array{
	int arr_size;
	int arr_capacity;
	long *arr_ptr;
}qarray;

int qarray_size();
bool qarray_empty();
void qarray_destroy();
void qarray_initialize();
DbOperator* qarray_next(Threadpool* p);
void qarray_push(DbOperator* dbo);
DbOperator* qarray_at(int index);

int tarray_size();
bool tarray_empty();
void tarray_destroy();
void tarray_initialize();
void tarray_push(Task* t);
Task* tarray_at(int index);

#endif