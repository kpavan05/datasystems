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


void db_fetch_long(Result* source, PositionVector* posvec, Result* result){
    long* source_payload = (long*)(source->payload);
    long* res_payload = malloc(posvec->num_tuples * sizeof(long));
    memset(res_payload, 0, posvec->num_tuples * sizeof(long));

    result->payload = res_payload;
    result->num_tuples = posvec->num_tuples;
    //result->posvec = posvec;

    size_t num_rows = source->num_tuples;
    for(size_t pos = 0 ; pos < num_rows; pos++){
        res_payload[pos] = source_payload[posvec->positions[pos]];
    }
}
void db_fetch_int(Result* source, PositionVector* posvec, Result* result){
    int* source_payload = (int*)(source->payload);

    int* res_payload = malloc(posvec->num_tuples * sizeof(int));
    memset(res_payload, 0, posvec->num_tuples * sizeof(int));
    result->payload = res_payload;
    result->num_tuples = posvec->num_tuples;
    //result->posvec = posvec;

    size_t num_rows = source->num_tuples;
    for(size_t pos = 0 ; pos < num_rows; pos++){
       res_payload[pos] = source_payload[posvec->positions[pos]];
    }
}

void db_fetch_result(Result* source, PositionVector* posvec, Result* result){
    result->base = source->base;
    result->data_type = source->data_type;

    if (result->data_type == LONG){
        db_fetch_long(source, posvec, result);
    }
    else {
        db_fetch_int(source, posvec, result);
    }   
}
void db_fetch_column(Column* column, PositionVector* posvec, Result* result){
    int* data = (int*)column->data;
    int* payload = malloc(posvec->num_tuples * sizeof(int));
    memset(payload, 0, posvec->num_tuples * sizeof(int));
    
    result->payload = payload;
    result->num_tuples = posvec->num_tuples;
    //result->posvec = posvec;
    result->data_type = INT;
    result->base = column;

    size_t num_rows = posvec->num_tuples;
    for(size_t pos = 0 ; pos < num_rows; pos++){
        payload[pos] = data[posvec->positions[pos]];     
    }
    /*
    for(size_t i = 0; i < result->num_tuples; i++){
        cs165_log(stdout, "%d:%d\n", i, payload[i]);
    }
    */
}
/*
    execute fetch_select and store intermediate result
*/
void db_fetch_select(PositionVector* source, PositionVector* dest){
    /*
    GeneralizedColumn* cur_column = dbo->operator_fields.select_operator.col_var;
    PositionVector* posvec = dbo->operator_fields.select_operator.pos_vec->column_pointer.posvec;
    
    Result *result = (Result*)malloc(sizeof(Result));
    assert(result != NULL);

    if (cur_column->column_type == COLUMN){
        db_fetch_column(cur_column->column_pointer.column, posvec, result);
    }
    else {
        db_fetch_result(cur_column->column_pointer.result, posvec, result);
    }
    */

    unsigned int nitems = dest->num_tuples;
    unsigned int* positions = NULL;
    positions = calloc(nitems, sizeof(unsigned int));

    for(unsigned int i= 0; i < nitems; i++){
        positions[i] = source->positions[dest->positions[i]];
    }
    free(dest->positions);
    dest->positions = NULL;

    dest->positions = positions;

}