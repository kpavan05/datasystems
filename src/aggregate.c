
#define _BSD_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include "cs165_api.h"
#include "message.h"
#include "utils.h"
#include "ioutils.h"
#include "client_context.h"

void db_calc_avg_i(size_t num_tuples,int* data, void* value){
    double res_val = 0.0;
    for(size_t i = 0; i < num_tuples; i++){
        res_val = res_val + data[i];
    }
    res_val = res_val/num_tuples;
    *(double*)value = res_val;
}
void db_calc_avg_l(size_t num_tuples,long* data, void* value){
    double res_val = 0.0;
    for(size_t i = 0; i < num_tuples; i++){
        res_val = res_val + data[i];
    }
    res_val = res_val/num_tuples;
    *(double*)value = res_val;
}
void db_calc_min_i(size_t num_tuples,int* data, void* value){
    int res_val = 0;
    for(size_t i = 0; i < num_tuples; i++){
        res_val = res_val < data[i] ? res_val : data[i];
    }
    *(int*)value = res_val;
}
void db_calc_min_l(size_t num_tuples,long* data, void* value){
    long res_val = 0;
    for(size_t i = 0; i < num_tuples; i++){
        res_val = res_val < data[i] ? res_val : data[i];
    }
    *(long*)value = res_val;
}
void db_calc_max_i(size_t num_tuples,int* data, void* value){
    int res_val = 0;
    for(size_t i = 0; i < num_tuples; i++){
        res_val = res_val > data[i] ? res_val : data[i];
    }
    *(int*)value = res_val;
}
void db_calc_max_l(size_t num_tuples,long* data, void* value){
    long res_val = 0;
    for(size_t i = 0; i < num_tuples; i++){
        res_val = res_val > data[i] ? res_val : data[i];
    }
    *(long*)value = res_val;
}
void db_calc_sum_i(size_t num_tuples,int* data, void* value){
    long res_val = 0;
    for(size_t i = 0; i < num_tuples; i++){
        res_val = res_val + data[i];
    }
    *(long*)value = res_val;
}
void db_calc_sum_l(size_t num_tuples,long* data, void* value){
    long res_val = 0;
    for(size_t i = 0; i < num_tuples; i++){
        res_val = res_val + data[i];
    }
    *(long*)value = res_val;
}

void db_calc_add_i(size_t nrows, int* data1, int* data2, Result* result){
    long *value = NULL;  
    value = (long*)malloc(nrows*sizeof(long));
    for( size_t i = 0; i < nrows; i++){
        value[i] = data1[i] + data2[i];
    }
    result->payload = value;
    result->num_tuples = nrows;
}
void db_calc_sub_i(size_t nrows, int* data1, int* data2, Result* result){
    long *value = NULL;
    value = (long*)malloc(nrows*sizeof(long));
    for( size_t i = 0; i < nrows; i++){
        value[i] = data1[i] - data2[i];
    }
    result->payload = value;
    result->num_tuples = nrows;
}
void db_calc_add_l(size_t nrows, long* data1, long* data2, Result* result){
    long *value = NULL;  
    value = (long*)malloc(nrows*sizeof(long));
    for( size_t i = 0; i < nrows; i++){
        value[i] = data1[i] + data2[i];
    }
    result->payload = value;
    result->num_tuples = nrows;
}
void db_calc_sub_l(size_t nrows, long* data1, long* data2, Result* result){
    long *value = NULL;
    value = (long*)malloc(nrows*sizeof(long));
    for( size_t i = 0; i < nrows; i++){
        value[i] = data1[i] - data2[i];
    }
    result->payload = value;
    result->num_tuples = nrows;
}
void db_calc_add_il(size_t nrows, long* data1, int* data2, Result* result){
    long *value = NULL;  
    value = (long*)malloc(nrows*sizeof(long));
    for( size_t i = 0; i < nrows; i++){
        value[i] = data1[i] + data2[i];
    }
    result->payload = value;
    result->num_tuples = nrows;
}
