#include "dsutil.h"
#include "batch.h"
#include "client_context.h"
#define ARRAY_CAPACITY 32


static qarray* task_array = NULL;
static qarray* query_array = NULL;


void qarray_initialize(){
	query_array = malloc(sizeof(qarray));
	query_array->arr_capacity = ARRAY_CAPACITY;
	query_array->arr_size = 0;
	query_array->arr_ptr = calloc(query_array->arr_capacity, sizeof(long));
	//query_array->arr_dbo = calloc(query_array->arr_capacity, sizeof(DbOperator*));
}

void qarray_push(DbOperator* dbo){
	if (query_array->arr_size == query_array->arr_capacity){
		query_array->arr_capacity *= 2;
		query_array->arr_ptr = realloc(query_array->arr_ptr, query_array->arr_capacity*sizeof(long));
		//query_array->arr_dbo = realloc(query_array->arr_dbo, 2*query_array->arr_capacity*sizeof(DbOperator*));
		
	}
	query_array->arr_ptr[query_array->arr_size] = (long)dbo;
	query_array->arr_size++;
}

DbOperator* qarray_next(Threadpool* pool){
	DbOperator* dbo = NULL;
	if (!pool) return NULL;

	pthread_mutex_lock(&lock);
	if (pool->cur_index < query_array->arr_size){		
		dbo = (DbOperator*)query_array->arr_ptr[pool->cur_index];
		pool->cur_index = pool->cur_index + 1;	
	}
	pthread_mutex_unlock(&lock);
	return dbo;
}

DbOperator* qarray_at(int index){
	if (index < 0 || index >= query_array->arr_size) return NULL;
	DbOperator* dbo = NULL;
	dbo = (DbOperator*)query_array->arr_ptr[index];
	return dbo;

}

void qarray_destroy(){
	DbOperator* dbo = NULL;
	int srvrun = 1;

	for(int i = 0; i < query_array->arr_size; i++){	
		dbo = (DbOperator*)query_array->arr_ptr[i];
		dbo->isbatched = false;
		db_operator_free(dbo, &srvrun);
		query_array->arr_ptr[i] = 0;
	}
    free(query_array->arr_ptr);
    query_array->arr_ptr = NULL;
	free(query_array);
	query_array = NULL;
}

bool qarray_empty(){
	return query_array->arr_size > 0 ?false :true;
}

int qarray_size(){ return query_array->arr_size;}




void tarray_initialize(){
	task_array = malloc(sizeof(qarray));
	task_array->arr_capacity = ARRAY_CAPACITY;
	task_array->arr_size = 0;
	task_array->arr_ptr = calloc(task_array->arr_capacity, sizeof(long));
	//query_array->arr_dbo = calloc(query_array->arr_capacity, sizeof(DbOperator*));
}

void tarray_push(Task* t){
	if (task_array->arr_size == task_array->arr_capacity){
		task_array->arr_capacity *= 2;
		task_array->arr_ptr = realloc(task_array->arr_ptr, task_array->arr_capacity*sizeof(long));
		//query_array->arr_dbo = realloc(query_array->arr_dbo, 2*query_array->arr_capacity*sizeof(DbOperator*));
		
	}
	task_array->arr_ptr[task_array->arr_size] = (long)t;
	task_array->arr_size++;
}


Task* tarray_at(int index){
	if (index < 0 || index >= task_array->arr_size) return NULL;
	Task* t = (Task*)task_array->arr_ptr[index];
	return t;

}

void tarray_destroy(){
    Task* t = NULL;
    GeneralizedColumn* pcol = NULL;
    int srvrun = 1;

    for(int i = 0; i < task_array->arr_size; i++){  
        t = (Task*)task_array->arr_ptr[i];
        pcol = lookup_generalized_column(t->dbo->operator_fields.select_operator.res_name,t->dbo->context);
        pcol->column_pointer.posvec->positions = realloc(t->positions, t->ntuples*sizeof(unsigned int));
        pcol->column_pointer.posvec->num_tuples = t->ntuples;
        t->dbo->isbatched = false;
        db_operator_free(t->dbo, &srvrun);
        free(t);
        task_array->arr_ptr[i] = 0;
    }
    free(task_array->arr_ptr);
    task_array->arr_ptr = NULL;
    free(task_array);
    task_array = NULL;
}

bool tarray_empty(){
	return task_array->arr_size > 0 ?false :true;
}

int tarray_size(){ return task_array->arr_size;}