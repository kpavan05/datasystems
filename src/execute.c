#define _BSD_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/time.h>
#include "cs165_api.h"
#include "message.h"
#include "utils.h"
#include "ioutils.h"
#include "dsutil.h"
#include "batch.h"
#include "ophelpers.h"
#include "indexop.h"
#include "join.h"
#include "client_context.h"


static void db_open(FILE *fp, ClientContext* context, Status *ret_status);
static Status db_print(DbOperator* dbo, message* msg);
static Status db_print_one(DbOperator *dbo, message* msg);
static void db_construct_msg(DbOperator* dbo, Status status, message* msg);

/*
    starts up database.
    looks at the default location and loads 
    internal data structures in memory
*/
Status db_startup(ClientContext* context){
    Status ret_status;
    FILE *fp = open_catalog("r");

    if (fp == NULL) {
       ret_status.code = OK;
       return ret_status;
    }
    db_open(fp, context, &ret_status);

    fclose(fp);
    fp = NULL;
    return ret_status;    
}

void db_open(FILE *fp, ClientContext* context, Status *ret_status){
    char line[MAX_DATA_LINE] ="";
    
    while(fgets(line, MAX_DATA_LINE, fp) != NULL){
        open_db_helper(line, context, ret_status);
    }
    db_restore_index();    
}

void db_restore_index(){
    if (current_db == NULL || !current_db->hasindex) return;

    int ntables = current_db->tables_size;
    for(int i = 0; i< ntables; i++){

        Table table = current_db->tables[i];
        int ncolumns = table.col_count;
        for(int j = 0; j < ncolumns; j++){

            Column* column = &(table.columns[j]);
            if (column->index == 0 ) continue;
            
            switch(column->index->type)
            {                
                case CLUSTER_BTREE: read_index(&table, column);break;
                case CLUSTER_SORT : db_sort_clustered(&table, column, true); break;
                case UNCLUSTER_BTREE: read_index(&table, column); break;
                case UNCLUSTER_SORT: read_index(&table, column); break;
                default: break;
            }
            /*
            switch(column->index->type)
            {                
                case CLUSTER_BTREE: db_btree_clustered(&table, column, true); break;
                case CLUSTER_SORT : db_sort_clustered(&table, column, true); break;
                case UNCLUSTER_BTREE: db_btree_unclustered(column); break;
                case UNCLUSTER_SORT: db_sort_unclustered(column); break;
                default: break;
            }
            */
        }
    }
}
/** 
    execute_DbOperator takes as input the DbOperator
    entry point to all query execution
 **/
Status execute_DbOperator(DbOperator* query, message* send_message) {
    Status status;
    status.code = OK;
    if (query == NULL) return status;

    switch(query->type){
        case CREATE:
            status = db_create(query);
            db_construct_msg(query, status, send_message);break;
        case LOAD:
            status = db_load(query);
            db_construct_msg(query, status, send_message);break;
        case INSERT:
            status = db_insert(query);
	        db_construct_msg(query, status, send_message);break;
        case SELECT:
            status = db_select(query);
            db_construct_msg(query, status, send_message); break;
        case FETCH:
            status = db_fetch(query);
            db_construct_msg(query, status, send_message); break;
        case SCALAR_OP:
            status = db_scalar_op(query);
            db_construct_msg(query, status, send_message); break;
        case ARITHMETIC_OP:
            status = db_arithmetic_op(query);
            db_construct_msg(query, status, send_message); break;
        case CLOSE:
            db_clear_client_context(query->context);
            query->context = NULL;
            db_shutdown(current_db);
            db_construct_msg(query, status, send_message);break;
        case BATCH_EXECUTE:
            status = db_batch_execute();
            //status.code = OK;
            //if (batch_run() == -1) status.code = ERROR;            
            db_construct_msg(query, status, send_message); break;
        case JOIN:
            status = db_join(query);
            db_construct_msg(query, status, send_message);break;
        case UPDATE:
            status = db_update(query);
            db_construct_msg(query, status, send_message);break;
        case DELETE:
            status = db_delete(query);
            db_construct_msg(query, status, send_message); break;
        case PRINT:
            {
                //status = db_print(query, send_message);break;
                
                if (query->operator_fields.print_operator.nhandles == 1){
                    status = db_print_one(query, send_message);break;
                } else {
                    status = db_print(query, send_message);break;
                }
                
            }
        default:
            break;
    }    
    return status;
}

Status db_create(DbOperator *dbo){
    Status create_status;
    switch(dbo->operator_fields.create_operator.op_type){
        case CREATE_DB:
            create_status = db_add(dbo->operator_fields.create_operator.db_name, true);
            break;
        case CREATE_TABLE:
            db_create_table(current_db, dbo->operator_fields.create_operator.tbl_name, dbo->operator_fields.create_operator.num_columns, &create_status);
            create_status.code = OK;
            break;
        case CREATE_COL:
            {
              Table *table = lookup_table(dbo->operator_fields.create_operator.tbl_name);   
              if (table == NULL){            
                cs165_log(stdout, "query unsupported. Bad table name");
                create_status.code = ERROR;
              }
              else {   
                Column* column = db_create_column(dbo->operator_fields.create_operator.col_name, table, false, &create_status);
                strcpy(column->full_name, dbo->operator_fields.create_operator.full_name);
                add_base_column_catalog_manager(dbo->operator_fields.create_operator.full_name, column, dbo->context);
                //add_base_column_catalog_manager1(dbo->operator_fields.create_operator.full_name, column, dbo->context, &dbo->context->chandle_table);
                create_status.code = OK;
              }
            }
            break;
        case CREATE_IDX:
            {
                GeneralizedColumn* gcol = lookup_generalized_column(dbo->operator_fields.create_operator.col_name, dbo->context);
                Column* column = gcol->column_pointer.column;
                if (column == NULL){cs165_log(stdout, "query unsupported. No column found"); create_status.code = ERROR;}

                column->index = malloc(sizeof(ColumnIndex));
                column->index->parent = column;
                column->index->type = dbo->operator_fields.create_operator.idx_type;
                
                create_status.code = OK;
            }
            break;
        default:
            break;
    }  
    return create_status;
}

void db_select_op(ComparatorType t, long int icompare, size_t num_rows, GeneralizedColumn* column, unsigned int* ntuples, PositionVector* res_vec){
    switch(t){
        case EQUAL:
            break;
        case LESS_THAN:
            db_select_less_op(icompare,num_rows,column, ntuples, res_vec);
            break;
        case LESS_THAN_OR_EQUAL:
            db_select_le_op(icompare,num_rows,column, ntuples, res_vec);
            break;
        case GREATER_THAN_OR_EQUAL:
            db_select_ge_op(icompare,num_rows,column, ntuples, res_vec);
            break; 
        case GREATER_THAN:
            db_select_greater_op(icompare,num_rows,column, ntuples, res_vec);
            break;
        case NONE:
            break;
        default:
            break;
    }
}
/*
    execute select statement
*/
Status db_select(DbOperator *dbo){
    Status ret_status;
    long int lo, hi; 
    unsigned int num_tuples = 0;
    size_t num_rows =0;
    ClientContext *context = dbo->context;    
    GeneralizedColumn* cur_column = NULL;
    PositionVector* res_pos = NULL;
    
    //struct timeval st, et;
    //gettimeofday(&st,NULL);

    //if query is batched then the intermediates have already been added to catalog manager
    if (!dbo->isbatched){ 
        res_pos = malloc(sizeof(PositionVector)); 
        add_position_vector_catalog_manager(dbo->operator_fields.select_operator.res_name, res_pos, context);
        //add_position_vector_catalog_manager1(dbo->operator_fields.select_operator.res_name, res_pos, context, &context->chandle_table);
    } else {
        GeneralizedColumn* gcol = lookup_generalized_column(dbo->operator_fields.select_operator.res_name, context);
        res_pos = gcol->column_pointer.posvec;
    }
    res_pos->positions = NULL;
    
    cur_column = dbo->operator_fields.select_operator.col_var;
    num_rows = cur_column->column_type == COLUMN ? cur_column->column_pointer.column->col_size: cur_column->column_pointer.result->num_tuples;

    ComparatorType t1 = dbo->operator_fields.select_operator.comparator.type1;
    ComparatorType t2 = dbo->operator_fields.select_operator.comparator.type2;
    lo = dbo->operator_fields.select_operator.comparator.p_low;
    hi = dbo->operator_fields.select_operator.comparator.p_high;

    if (t1 != NONE && t2 != NONE){
        db_select_ge_less_op(lo, hi, num_rows, cur_column, &num_tuples, res_pos);
    }
    else if (t1 != NONE && lo != INVALID_VAL){
        db_select_op(t1, lo, num_rows, cur_column, &num_tuples, res_pos);
    }
    else if (t2 != NONE && hi != INVALID_VAL){
        db_select_op(t2, hi, num_rows, cur_column, &num_tuples, res_pos);
    }
    res_pos->num_tuples = num_tuples;
    if (dbo->operator_fields.select_operator.pos_vec != NULL){
        db_fetch_select(dbo->operator_fields.select_operator.pos_vec->column_pointer.posvec, res_pos);
    }
    
    //gettimeofday(&et,NULL);
    //long elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec) ;  
    //cs165_log(stdout, "--Time taken for %s run %ld usec\n", dbo->operator_fields.select_operator.res_name, elapsed);
    /*
    cs165_log(stdout, "POSITION VECTOR:\n");
    for(int i = 0; i < nitems; i++){
        cs165_log(stdout, "%d:%d\n", i, res_pos->positions[i]);
    }
    */   
    ret_status.code = OK;
    return ret_status; 
}

/*
    execute fetch statement
*/
Status db_fetch(DbOperator *dbo){
    Status ret_status;
    ClientContext *context = dbo->context;
    Result *result = (Result*)malloc(sizeof(Result));
    assert(result != NULL);

    //add_result_vector_catalog_manager1(dbo->operator_fields.fetch_operator.res_name, result,context, &(context->chandle_table));
    add_result_vector_catalog_manager(dbo->operator_fields.fetch_operator.res_name, result,context);
    GeneralizedColumn* cur_column = dbo->operator_fields.fetch_operator.col_var;
    PositionVector* posvec = dbo->operator_fields.fetch_operator.vec_pos->column_pointer.posvec;

    if (cur_column->column_type == COLUMN){
        db_fetch_column(cur_column->column_pointer.column, posvec, result);
    }
    else {
        db_fetch_result(cur_column->column_pointer.result, posvec, result);
    }   
    ret_status.code = OK;
    return ret_status; 
}
/*
    execute load statement
    loads data from an external file into DB files
    similar to bulk insert into a table.
*/
Status db_load(DbOperator *dbo){
    struct Status ret_status;
    char* tokenizer_copy;
    char* to_free;
    char* line;
    char* token;
    int** values;
    size_t i= 0, j = 0;
    size_t row_num = 0, capacity = 0;
    Table *table = NULL;

    assert(dbo->type == LOAD);
    table = dbo->operator_fields.load_operator.table ;
        
    tokenizer_copy = to_free = malloc((strlen(dbo->operator_fields.load_operator.data)+1) * sizeof(char));
    strcpy(tokenizer_copy, dbo->operator_fields.load_operator.data);

    size_t nlines = find_num_tokens(tokenizer_copy, '$');
    if (nlines > 1) nlines--;
    size_t ncolumns = table->col_count;

    values = calloc(nlines, sizeof(int*));
    for(i = 0; i < nlines; i++){
        values[i] = malloc(ncolumns*sizeof(int));
    }

    i = 0;
    while ((line = strsep(&tokenizer_copy, "$")) != NULL && i < nlines) {
        j = 0;
        while((token = strsep(&line, ",")) != NULL){
            values[i][j] = atoi(token);
            j++;
        }
        i++;   
    }


    table->table_length += nlines;
    for(j = 0 ; j < ncolumns; j++){

        Column *column = (Column*)(dbo->operator_fields.load_operator.ptr_arr[j]);
        capacity = column->col_capacity;
        row_num = column->col_size;
        for(i = 0; i < nlines; i++){
            if (row_num == capacity){                
                column->data = (int*)realloc(column->data, 2*capacity*sizeof(int));
                capacity *= 2;
            }
            column->data[row_num] = values[i][j];
            row_num++;
        } 
        column->col_size = row_num;
        column->col_capacity = capacity;
    }
    
    for (i = 0; i < nlines; i++)
        free(values[i]);
    
    free(values);
    free(to_free);
    

    if (dbo->operator_fields.load_operator.rstate == READ_END){
        for(j = 0 ; j < ncolumns; j++){
            Column *column = (Column*)(dbo->operator_fields.load_operator.ptr_arr[j]);
            if (column->index) db_create_index(table, column);
        }
    }
    table = NULL;
    ret_status.code = OK;
    return ret_status;
}

Status db_create_index(Table* table, Column* column){
    Status ret_status;
    switch(column->index->type){
        case CLUSTER_BTREE:
            ret_status = db_btree_clustered(table, column, false);
            break;
        case CLUSTER_SORT:
            ret_status = db_sort_clustered(table, column, false); 
            break;
        case UNCLUSTER_BTREE:
            ret_status = db_btree_unclustered(column);
            break;
        case UNCLUSTER_SORT:
            ret_status = db_sort_unclustered(column);
            break;
        default :
            break;
    }
    ret_status.code = OK;
    return ret_status;
}
void db_increase_capacity(Table* table){
    Column* column = NULL;

    size_t ncolumns = table->col_count;
    for(size_t i = 0; i < ncolumns; i++){
        column = &table->columns[i];

        column->col_capacity *= 2; 
        column->data = realloc(column->data, column->col_capacity*sizeof(int));
        if (column->index ){
           if (column->index->type == UNCLUSTER_SORT){
              column->index->idxdata.sortidx.copy_data = realloc(column->index->idxdata.sortidx.copy_data, column->col_capacity*sizeof(int));
              column->index->idxdata.sortidx.pv->positions = realloc(column->index->idxdata.sortidx.pv->positions, column->col_capacity*sizeof(unsigned int));
           }
           if (column->index->type == CLUSTER_SORT) 
             column->index->idxdata.sortidx.pv->positions = realloc(column->index->idxdata.sortidx.pv->positions, column->col_capacity*sizeof(unsigned int));        
        }            
    }     
}
/*
    execute insert statement
*/
Status db_insert(DbOperator *dbo){
    Status ret_status;
    bool binserted = false;
    Table *table = dbo->operator_fields.insert_operator.table;
    size_t capacity = table->columns[0].col_capacity;
    size_t row_num = table->table_length;
    size_t ncolumns = table->col_count;

    if (row_num == capacity) db_increase_capacity(table); 
    
    
    for(size_t i = 0; i < ncolumns; i++){
        if (table->columns[i].index != NULL && 
           (table->columns[i].index->type == CLUSTER_BTREE || table->columns[i].index->type == CLUSTER_SORT)){
            db_insert_to_index(table, &table->columns[i], i, dbo->operator_fields.insert_operator.values, UINT_MAX);
            binserted = true;
        }
    }
    if (!binserted){
        for(size_t i = 0; i < ncolumns; i++){
               
            table->columns[i].data[row_num] = dbo->operator_fields.insert_operator.values[i]; 
            
            if (table->columns[i].index != NULL && 
               (table->columns[i].index->type == UNCLUSTER_BTREE || table->columns[i].index->type == UNCLUSTER_SORT)){
                db_insert_to_index(table, &table->columns[i], i, dbo->operator_fields.insert_operator.values, row_num);               
            }           
            table->columns[i].col_size++;
        } 
    }    
    table->table_length++;
    table = NULL;
    ret_status.code = OK;
    return ret_status;
}
/*
    execute update statement
*/
Status db_update(DbOperator* dbo){
    Status ret_status; 
    Table* table = dbo->operator_fields.update_operator.table;
    Column* column = dbo->operator_fields.update_operator.col_var;
    PositionVector* pv = dbo->operator_fields.update_operator.pos_vec->column_pointer.posvec;
    int value = dbo->operator_fields.update_operator.update_value;
    
    if (column->index != NULL && 
       (column->index->type == CLUSTER_SORT || column->index->type == CLUSTER_BTREE)){
        db_update_index(table, column, pv, value);
    } else {
        int* data = column->data;
        if (column->index != NULL){
            db_update_index(table, column, pv, value);
        }
        for(unsigned int i = 0; i < pv->num_tuples; i++){
            data[pv->positions[i]] = value;
        }        
    }
    ret_status.code = OK;
    return ret_status;
}
/*
    execute delete statement
*/
Status db_delete(DbOperator* dbo){
    Status ret_status;
    bool bdeleted = false;
    unsigned int  numrows;    
    Table* table = dbo->operator_fields.delete_operator.table;
    PositionVector* pv = dbo->operator_fields.delete_operator.pos_vec->column_pointer.posvec;
    
    size_t ncolumns = table->col_count;
    for(size_t i = 0; i < ncolumns; i++){
        if (table->columns[i].index != NULL && 
           (table->columns[i].index->type == CLUSTER_BTREE || table->columns[i].index->type == CLUSTER_SORT)){
            db_delete_index(table, &table->columns[i], pv);
            bdeleted = true;
        }
    }
    if (!bdeleted){
        int count = 0;
        int* curdata = NULL;
        Column* curcolumn = NULL;
        for(size_t i = 0; i < ncolumns; i++){
            curcolumn = &table->columns[i];
            curdata = curcolumn->data; 
            numrows =  curcolumn->col_size;
            if (curcolumn->index != NULL && 
               (curcolumn->index->type == UNCLUSTER_BTREE || curcolumn->index->type == UNCLUSTER_SORT)){
                db_delete_index(table, curcolumn, pv);               
            } 
            for(unsigned int pos = 0; pos < numrows; pos++){
                count += (pv->positions[count] == pos);
                curdata[pos] = curdata[pos + count];
            }
            curcolumn->col_size = curcolumn->col_size - pv->num_tuples;  
        } 
    }    
    table->table_length = table->table_length - pv->num_tuples;
    table = NULL;
    ret_status.code = OK;
    return ret_status;

}
/*
    execute avg, sum, min, max operators
*/
Status db_scalar_op(DbOperator* dbo){
    Status ret_status;
    int* idata = NULL;
    long* ldata = NULL;
    size_t num_tuples = 0;
    DataType dt;

    ret_status.code = OK;
    ClientContext *context = dbo->context;
    GeneralizedColumn* col_var = dbo->operator_fields.scalar_operator.col_var;

    Scalar *scalar = (Scalar*)malloc(sizeof(Scalar));
    assert(scalar != NULL);
    scalar->data_type = INT;
    scalar->svalue = NULL;

    add_scalar_catalog_manager(dbo->operator_fields.scalar_operator.res_name, scalar, context);
    
    if (col_var->column_type == COLUMN){
        num_tuples = col_var->column_pointer.column->col_size;
        dt = INT;
        idata = col_var->column_pointer.column->data;
    }
    else if (col_var->column_type == RESULT){
        num_tuples = col_var->column_pointer.result->num_tuples;
        dt = col_var->column_pointer.result->data_type;
        if (dt == LONG){
            scalar->data_type = LONG;
            ldata = col_var->column_pointer.result->payload;
        }
        else{
            scalar->data_type = INT;    
            idata = col_var->column_pointer.result->payload;
        }
    }
    if (num_tuples == 0) return ret_status;
    switch(dbo->operator_fields.scalar_operator.op_type){
        case AVG:
        {
            scalar->data_type = DOUBLE;
            scalar->svalue = malloc(sizeof(double));
            if (ldata != NULL){            
                db_calc_avg_l(num_tuples, ldata, scalar->svalue);
            }
            else {
                db_calc_avg_i(num_tuples, idata, scalar->svalue);
            }
        }
            //sprintf(dbo->msg, "AVERAGE : %f\n", *(float*)scalar->svalue);
        break;
        case SUM:
        {
            scalar->data_type = LONG;
            scalar->svalue = malloc(sizeof(long)); 
            if (ldata != NULL){                 
                db_calc_sum_l(num_tuples, ldata, scalar->svalue);
            }
            else {
                db_calc_sum_i(num_tuples, idata, scalar->svalue);
            }
        }
        break;
        case MIN:
        {   
            if (ldata != NULL){ 
                scalar->svalue = malloc(sizeof(long)); 
                db_calc_min_l(num_tuples, ldata, scalar->svalue);
            }
            else{
                scalar->svalue = malloc(sizeof(int));
                db_calc_min_i(num_tuples, idata, scalar->svalue);
            }
            //sprintf(dbo->msg, "MINIMUM VALUE :%d\n", *(int*)scalar->svalue);
        }
        break;
        case MAX:
        {
            if (ldata != NULL){
                scalar->svalue = malloc(sizeof(long)); 
                db_calc_max_l(num_tuples, ldata, scalar->svalue);
            }
            else{
                scalar->svalue = malloc(sizeof(int));
                db_calc_max_i(num_tuples, idata, scalar->svalue);
            }
            //sprintf(dbo->msg, "MAXIMUM VALUE :%d\n", *(int*)scalar->svalue);
        }
        break;
        default:
            break;
    }
    ret_status.code = OK;
    return ret_status;
}


/*
    execute add, sub operators
*/
Status db_arithmetic_op(DbOperator* dbo){
    Status ret_status;
    int* idata1 = NULL;
    int* idata2 = NULL;
    long* ldata1 = NULL;
    long* ldata2 = NULL;
    size_t num_tuples1 = 0, num_tuples2 = 0;

    ClientContext *context = dbo->context;
    GeneralizedColumn* col1, *col2;

    Result *result = (Result*)malloc(sizeof(Result));
    assert(result != NULL);
    result->data_type = LONG;
    result->payload = NULL;

    add_result_vector_catalog_manager(dbo->operator_fields.arithmetic_operator.res_name, result, context);
    col1 = dbo->operator_fields.arithmetic_operator.col_var1;
    col2 = dbo->operator_fields.arithmetic_operator.col_var2;

    if (col1->column_type== COLUMN){
        idata1 = col1->column_pointer.column->data;
        result->base = col1->column_pointer.column;
    }

    if (col2->column_type== COLUMN)
        idata2 = col1->column_pointer.column->data;

    if (col1->column_type== RESULT){
        if (col1->column_pointer.result->data_type == LONG)
            ldata1 = col1->column_pointer.result->payload;
        else
            idata1 = col1->column_pointer.result->payload;

        result->base = col1->column_pointer.result->base;
    }
    if (col2->column_type== RESULT){
        if (col2->column_pointer.result->data_type == LONG)
            ldata2 = col2->column_pointer.result->payload;    
        else
            idata2 = col2->column_pointer.result->payload;
    }

    num_tuples1 = col1->column_type== COLUMN ? col1->column_pointer.column->col_size : col1->column_pointer.result->num_tuples;
    num_tuples2 = col2->column_type== COLUMN ? col2->column_pointer.column->col_size : col2->column_pointer.result->num_tuples;
    if (num_tuples1 != num_tuples2){
        ret_status.code = ERROR;
        return ret_status;
    }
    switch(dbo->operator_fields.arithmetic_operator.op_type){
        case ADD:
        {
            if (ldata1 && ldata2)            
                db_calc_add_l(num_tuples1, ldata1, ldata2, result);
            else if (idata1 && ldata2) 
                db_calc_add_il(num_tuples1, ldata2, idata1, result);
            else if (ldata1 && idata2)
                db_calc_add_il(num_tuples1, ldata1, idata2, result);
            else
                db_calc_add_i(num_tuples1, idata1, idata2, result);
            break;
        }
        case SUB:
        {
            if (ldata1 && ldata2)            
                db_calc_sub_l(num_tuples1, ldata1, ldata2, result);
            else
                db_calc_sub_i(num_tuples1, idata1, idata2, result);
            break;
        }
        default:
            break;
    }
    ret_status.code = OK;
    return ret_status;
}
/*
    execute the operation in between batch_queries and batch_execute.
*/
Status db_batch_execute(){
    Status ret_status;
    /*
    struct timeval st, et;
    gettimeofday(&st,NULL);
    */
    batch_run();
    /*
    gettimeofday(&et,NULL);
    long elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    cs165_log(stdout, "--Time taken for batch run %ld usec\n", elapsed);
    */
    ret_status.code = OK;
    return ret_status;
}

Status db_join(DbOperator *dbo){
    Status ret_status;   
    ClientContext *context = dbo->context;    
    PositionVector* res1 = NULL;
    PositionVector* res2 = NULL;

    size_t len1 = dbo->operator_fields.join_operator.sel1->column_pointer.posvec->num_tuples;
    size_t len2 = dbo->operator_fields.join_operator.sel2->column_pointer.posvec->num_tuples;
    
    PositionVector* s1 = len1 > len2 ? dbo->operator_fields.join_operator.sel1->column_pointer.posvec : dbo->operator_fields.join_operator.sel2->column_pointer.posvec;
    Result* v1 = len1 > len2 ? dbo->operator_fields.join_operator.val1->column_pointer.result : dbo->operator_fields.join_operator.val2->column_pointer.result;
    PositionVector* s2 = len1 > len2 ? dbo->operator_fields.join_operator.sel2->column_pointer.posvec : dbo->operator_fields.join_operator.sel1->column_pointer.posvec;
    Result* v2 = len1 > len2 ? dbo->operator_fields.join_operator.val2->column_pointer.result : dbo->operator_fields.join_operator.val1->column_pointer.result;
    
    res1 = malloc(sizeof(PositionVector)); 
    add_position_vector_catalog_manager(dbo->operator_fields.join_operator.res_name1, res1, context);
    res2 = malloc(sizeof(PositionVector)); 
    add_position_vector_catalog_manager(dbo->operator_fields.join_operator.res_name2, res2, context);  

    PositionVector* r1 = len1 > len2 ? res1 : res2;
    PositionVector* r2 = len1 > len2 ? res2 : res1;
    if (v1->data_type == LONG){
        long* payload1 = v1->payload;
        long* payload2 = v2->payload;
        dbo->operator_fields.join_operator.optype == NESTED_LOOP ? db_nested_join_l(payload1, payload2, s1, s2, r1, r2) : db_hash_join_l(payload1, payload2, s1,s2, r1, r2);
    } else {
        int* payload1 = v1->payload;
        int* payload2 = v2->payload;
        switch(dbo->operator_fields.join_operator.optype){
            case NESTED_LOOP: 
                db_nested_join_i(payload1, payload2, s1, s2, r1, r2); break;
            case HASH_JOIN: 
                db_hash_join_i(payload1, payload2, s1,s2, r1, r2); break;
            case SORT_MERGE: 
                db_sort_join_i(payload1, payload2, s1, s2, r1, r2);break;
            case GRACE_JOIN:
                db_grace_join_i(payload1, payload2, s1,s2, r1, r2); break;
            default: break;
        }
    }
    ret_status.code = OK;
    return ret_status; 
}

/*
    clean up all the handles held by client_context object
*/
void db_clear_client_context(ClientContext* context){

    if (context == NULL) return;
    int nhandles = context->chandles_in_use;
    for(int i = 0; i < nhandles; i++){
        if (context->chandle_table[i].generalized_column.column_type == COLUMN){
            context->chandle_table[i].generalized_column.column_pointer.column = NULL;
        } else if (context->chandle_table[i].generalized_column.column_type == POSITION){
            free(context->chandle_table[i].generalized_column.column_pointer.posvec->positions);
            free(context->chandle_table[i].generalized_column.column_pointer.posvec);
        } else if (context->chandle_table[i].generalized_column.column_type == RESULT){
            context->chandle_table[i].generalized_column.column_pointer.result->base = NULL;
            free(context->chandle_table[i].generalized_column.column_pointer.result->payload);
            free(context->chandle_table[i].generalized_column.column_pointer.result);
        } else if (context->chandle_table[i].generalized_column.column_type == SCALAR){
            free(context->chandle_table[i].generalized_column.column_pointer.scalar->svalue);
            free(context->chandle_table[i].generalized_column.column_pointer.scalar);
        }
    }
    free(context->chandle_table);
}
/*
    execute shutdown statement
*/
Status db_shutdown(Db* db){
    struct Status ret_status;
    char data[MAX_DATA_LINE];

    //write metadata to catalog file
    FILE *fp = open_catalog("w");
    if (fp == NULL){
        ret_status.code = ERROR;
        return ret_status;
    }
    sprintf(data, "Dbname:%s\n", db->name);
    fputs(data, fp);
    
    open_db_folder(db->name, NULL, true);
    int ntables = db->tables_size;
    for(int i = 0; i < ntables; i++){
        sprintf(data, "Table:%s,%zu,%zu\n", db->tables[i].name, db->tables[i].col_count, db->tables[i].table_length);
        fputs(data, fp);
        open_db_folder(db->tables[i].name, db->name, true);
        commit_table(fp, &db->tables[i]);
    }

    free(db->tables);
    db->tables = NULL;

    fflush(fp);
    fclose(fp);
    fp = NULL;
    ret_status.code = OK;
    return ret_status;
}

Status shutdown_server(){
    struct Status ret_status;
    ret_status.code = OK;
    if (current_db){
        ret_status = db_shutdown(current_db);
    }   
    return ret_status;
}
/*
    free the dboperator
*/
void db_operator_free(DbOperator* dbo, int* is_srv_run){
    if (dbo == NULL) return;
    if (dbo->isbatched) return;

    switch(dbo->type){
        case CREATE:
            break;
        case LOAD:
            dbo->operator_fields.load_operator.table = NULL;
            free(dbo->operator_fields.load_operator.ptr_arr);
            free(dbo->operator_fields.load_operator.data);
            break;
        case INSERT:
            dbo->operator_fields.insert_operator.table = NULL;
            free(dbo->operator_fields.insert_operator.values);
            break;
        case SELECT:
            dbo->operator_fields.select_operator.col_var = NULL;
            dbo->operator_fields.select_operator.pos_vec = NULL;
            dbo->operator_fields.select_operator.comparator.gen_col = NULL;
            break;;
        case FETCH:
            dbo->operator_fields.fetch_operator.col_var = NULL;
            dbo->operator_fields.fetch_operator.vec_pos = NULL;
            break;
        case SCALAR_OP:
            dbo->operator_fields.scalar_operator.col_var = NULL;
            break;
        case ARITHMETIC_OP:
            {
            dbo->operator_fields.arithmetic_operator.col_var1 = NULL; 
            dbo->operator_fields.arithmetic_operator.col_var2 = NULL;    
            }   
            break;
        case PRINT:
            {
            int nhandles = dbo->operator_fields.print_operator.nhandles;
            for (int i = 0; i < nhandles; i++){
                if (dbo->operator_fields.print_operator.phandles[i].generalized_column.column_type == COLUMN){
                    dbo->operator_fields.print_operator.phandles[i].generalized_column.column_pointer.column = NULL;
                } else if (dbo->operator_fields.print_operator.phandles[i].generalized_column.column_type == RESULT){
                    dbo->operator_fields.print_operator.phandles[i].generalized_column.column_pointer.result = NULL;
                } else if (dbo->operator_fields.print_operator.phandles[i].generalized_column.column_type == POSITION){
                    dbo->operator_fields.print_operator.phandles[i].generalized_column.column_pointer.posvec = NULL;
                } else if (dbo->operator_fields.print_operator.phandles[i].generalized_column.column_type == SCALAR){
                    dbo->operator_fields.print_operator.phandles[i].generalized_column.column_pointer.scalar = NULL;
                }
            }
            free(dbo->operator_fields.print_operator.phandles);
            }
            break;
        case CLOSE:
            *is_srv_run = 0;            
            break;
        case BATCH_EXECUTE:
            //qarray_destroy();
            break;
        default:
            break;
    }
    dbo->client_fd = -1;
    dbo->context = NULL;
    free(dbo);
    dbo = NULL;
}

void db_construct_msg(DbOperator* query, Status status, message* msg) {
    if (query == NULL) return;

    msg->payload = malloc(SMALL_BUF_SIZE*sizeof(char));
    if (query->isbatched){
        strcpy(msg->payload, "-- QUERY BATCHED"); 
    }
    else {
    switch(query->type){
        case CREATE:
            {
            if (status.code != OK)         
                strcpy(msg->payload, "-- CREATE: EXECUTION ERROR");       
            else
                strcpy(msg->payload, "-- CREATE: SUCCESSFUL");
            }
            break;
        case LOAD:
            {
            if (status.code != OK)         
                strcpy(msg->payload, "-- LOAD: EXECUTION ERROR");       
            else
                strcpy(msg->payload, "-- LOAD: SUCCESSFUL");
            }
            break;
        case INSERT:
            {
            if (status.code != OK)         
                strcpy(msg->payload, "-- INSERT: EXECUTION ERROR");      
            else
                strcpy(msg->payload, "-- INSERT: SUCCESSFUL");
            }
            break;
        case UPDATE:
            {
            if (status.code != OK)         
                strcpy(msg->payload, "-- UPDATE: EXECUTION ERROR");      
            else
                strcpy(msg->payload, "-- UPDATE: SUCCESSFUL");
            }
            break;
        case SELECT:
            {
            if (status.code != OK)         
                strcpy(msg->payload, "-- SELECT: EXECUTION ERROR");       
            else
                strcpy(msg->payload, "-- SELECT: SUCCESSFUL");
            }
            break;
        case FETCH:
            {
            if (status.code != OK)         
                strcpy(msg->payload, "-- FETCH: EXECUTION ERROR");       
            else
                strcpy(msg->payload, "-- FETCH: SUCCESSFUL");
            }
            break;
        case SCALAR_OP:
            {
            if (status.code != OK)         
                strcpy(msg->payload, "-- SCALAR OP: EXECUTION ERROR");       
            else
                strcpy(msg->payload, "-- SCALAR OP: SUCCESSFUL");
            }
            break;
        case ARITHMETIC_OP:
            {
            if (status.code != OK)         
                strcpy(msg->payload, "-- ARITHMETIC OP: EXECUTION ERROR");       
            else
                strcpy(msg->payload, "-- ARITHMETIC OP: SUCCESSFUL");
            }
            break;
        case JOIN:
            {
            if (status.code != OK)         
                strcpy(msg->payload, "-- JOIN: EXECUTION ERROR");       
            else
                strcpy(msg->payload, "-- JOIN: SUCCESSFUL");
            }
            break;
        case CLOSE:
            strcpy(msg->payload, "-- DB SHUTDOWN SUCCESSFUL");
            break;
        case BATCH_EXECUTE:
            strcpy(msg->payload, "--BATCH EXECUTION DONE");
            break;
        case DELETE:
            {
            if (status.code != OK)         
                strcpy(msg->payload, "-- DELETE: EXECUTION ERROR");      
            else
                strcpy(msg->payload, "-- DELETE: SUCCESSFUL");
            }
            break;
         default:
            break;
    }
    }
    msg->payload[strlen(msg->payload)] = '\0'; 
    msg->length = strlen(msg->payload);
    msg->status = OK_DONE;
}

size_t print_get_num_rows(DbOperator* dbo){
    size_t num_rows = 0, max_rows = 0;
    int nhandles = dbo->operator_fields.print_operator.nhandles;

    for(int i = 0; i < nhandles; i++){
        GeneralizedColumn gcol = dbo->operator_fields.print_operator.phandles[i].generalized_column;
        if (gcol.column_type == COLUMN){
            num_rows = gcol.column_pointer.column->col_size;
        }
        else if (gcol.column_type == RESULT){
            num_rows = gcol.column_pointer.result->num_tuples;
        }
        else if (gcol.column_type == SCALAR){
            num_rows = gcol.column_pointer.scalar->svalue != NULL ? 1 :0;
        }
        max_rows = max_rows < num_rows ? num_rows : max_rows;
    }
    return max_rows;
}
/*************************************************************************************
send the message as text for multiple display columns
// ***********************************************************************************/
void print_value(GeneralizedColumn* gen_col, size_t index, char* cvalue, bool bdelimit){
    void *payload = NULL;
    int *idata = NULL;
    double *ddata = NULL;
    long* ldata = NULL;
    DataType t = gen_col->column_type == RESULT ? gen_col->column_pointer.result->data_type: gen_col->column_pointer.scalar->data_type;
    payload = gen_col->column_type == RESULT ? gen_col->column_pointer.result->payload :gen_col->column_pointer.scalar->svalue; 

    if (t == FLOAT || t == DOUBLE){
        ddata = payload;
        if (bdelimit) sprintf(cvalue, ",%.2f", ddata[index]);
        else sprintf(cvalue, "%.2f", ddata[index]);
    } else if (t == INT){
        idata = payload;
        if (bdelimit) sprintf(cvalue, ",%d", idata[index]);
        else sprintf(cvalue, "%d", idata[index]);
    } else if (t == LONG){
        ldata = payload;
        if (bdelimit) sprintf(cvalue, ",%ld", ldata[index]);
        else sprintf(cvalue, "%ld", ldata[index]);
    }
}

Status db_print(DbOperator *dbo, message* msg){ 
    int offset = 0;
    int cur_len = 0;
    Status ret_status;
    char line[MAX_LINE_SIZE] = "";
    //char print_state[8] ="";
    size_t i = 0;
    int nhandles = dbo->operator_fields.print_operator.nhandles;
    GeneralizedColumn* gen_col = NULL;
  
    ret_status.code = OK;
    cur_len = 8;
    static size_t num_rows = 0;
    static size_t start = 0;
    if (start == 0){
        num_rows = print_get_num_rows(dbo);
        msg->payload = malloc(DEFAULT_QUERY_BUFFER_SIZE*sizeof(char) + 1);
        if (num_rows == 0) {
            sprintf(msg->payload,"$PRNT:%d",ret_status.code);
            msg->payload[strlen(msg->payload)] = '\0';
            msg->length = strlen(msg->payload);
            msg->status = OK_DONE;
            return ret_status;
        }
    }
    memset(msg->payload, 0, DEFAULT_QUERY_BUFFER_SIZE*sizeof(char));

    for(i = start; i < num_rows; i++){
        memset(line, 0, MAX_LINE_SIZE*sizeof(char));
        line[0] = '\0'; offset = 0;

        for(int j = 0; j < nhandles; j++){
            gen_col = &dbo->operator_fields.print_operator.phandles[j].generalized_column;
            print_value(gen_col, i, line+offset, j != 0);
            offset = strlen(line);
        }
        line[offset] = '\n';
        line[strlen(line)] = '\0';
        if (cur_len + strlen(line) > DEFAULT_QUERY_BUFFER_SIZE) {
            start = i;
            ret_status.code = INCOMPLETE;
            break;
        }
        else{
            strcat(msg->payload, line);
            cur_len = cur_len +  strlen(line);
        }
    }
    if (i == num_rows){
        start = 0;   
        ret_status.code = OK;
    }
    sprintf(msg->payload + strlen(msg->payload) -1,"$PRNT:%d",ret_status.code);
    msg->payload[strlen(msg->payload)] = '\0';
    msg->length = strlen(msg->payload);
    msg->status = OK_DONE;
    return ret_status;
}


/*************************************************************************************
    send the message in byte stream rather than text 
    (called when the print has one column as parameter)
// ***********************************************************************************/
char end_token1[] = "$PRNT:";
char end_token[] = "$BPRNT:";
char start_token[] = "$DTYPE:";
char datalen_token[] = "$DLEN:";

void print_values_scalar(message* m, Scalar* scalar, size_t start, size_t end){
    int offset;
    int type = scalar->data_type;
    int len = end-start;

    memcpy(&m->payload[m->length], &type, sizeof(int));
    m->length += sizeof(int);

    memcpy(&m->payload[m->length], datalen_token, strlen(datalen_token));
    m->length += strlen(datalen_token);

    memcpy(&m->payload[m->length], &len, sizeof(int));
    m->length +=  sizeof(int);

    offset = m->length;
    if (type == DOUBLE){
        memcpy(&m->payload[offset], (unsigned char*)scalar->svalue, sizeof(double));
        offset += sizeof(double);
    } else if (type == INT) {
        memcpy(&m->payload[offset], (unsigned char*)scalar->svalue, sizeof(int));
        offset += sizeof(int);
    } else if (type == LONG) {
        memcpy(&m->payload[offset], (unsigned char*)scalar->svalue, sizeof(long));
        offset += sizeof(long);
    }    
    m->length = offset;
}

void print_values_result(message* m, Result* result, size_t start, size_t end){
    int offset;
    int *idata = NULL;
    double *ddata = NULL;
    long* ldata = NULL;
    int type = result->data_type;
    int len = end-start;

    if (result->data_type == FLOAT || result->data_type == DOUBLE){
        ddata = result->payload;
        type = DOUBLE;

        memcpy(&m->payload[m->length], &type, sizeof(int));
        m->length += sizeof(int);
        memcpy(&m->payload[m->length], datalen_token, strlen(datalen_token));
        m->length += strlen(datalen_token);
        memcpy(&m->payload[m->length], &len, sizeof(int));
        m->length += sizeof(int);

        offset = m->length;
        for(size_t i = start; i < end; i++){
            memcpy(&m->payload[offset], &ddata[i], sizeof(double));
            offset += sizeof(double);
        }
        m->length = offset;

    } else if (result->data_type == INT){
        idata = result->payload;
        type = INT;

        memcpy(&m->payload[m->length], &type, sizeof(int));
        m->length += sizeof(int);
        memcpy(&m->payload[m->length], datalen_token, strlen(datalen_token));
        m->length += strlen(datalen_token);
        memcpy(&m->payload[m->length], &len, sizeof(int));
        m->length +=  sizeof(int);

        offset = m->length;
        for(size_t i = start; i < end; i++){
            memcpy(&m->payload[offset], &idata[i], sizeof(int));
            offset += sizeof(int);
        }
        m->length = offset;

    } else if (result->data_type == LONG){
        ldata = result->payload;
        type = LONG;
        memcpy(&m->payload[m->length], &type, sizeof(int));
        m->length += sizeof(int);
        memcpy(&m->payload[m->length], datalen_token, strlen(datalen_token));
        m->length += strlen(datalen_token);
        memcpy(&m->payload[m->length], &len, sizeof(int));
        m->length += sizeof(int);

        offset = m->length;
        for(size_t i = start; i < end; i++){
            memcpy(&m->payload[offset], &ldata[i], sizeof(long));
            offset += sizeof(long);
        }
        m->length = offset;
    }
}

void print_values_column(message* m, Column* column, size_t start, size_t end){
    int offset;
    int type = INT;
    int len = end-start;
    int *idata = column->data;

    memcpy(&m->payload[m->length], &type, sizeof(int));
    m->length += sizeof(int);
    memcpy(&m->payload[m->length], datalen_token, strlen(datalen_token));
    m->length += strlen(datalen_token);
    memcpy(&m->payload[m->length], &len, sizeof(int));
    m->length += sizeof(int);

    offset = m->length;
    for(size_t i = start; i < end; i++){
        memcpy(&m->payload[offset], &idata[i], sizeof(int));
        offset += sizeof(int);
    }
    m->length = offset;
}

Status db_print_one(DbOperator *dbo, message* msg){ 
    Status ret_status;
    int ncode = 0;
    static size_t num_rows = 0;
    static size_t start = 0;

    GeneralizedColumn* gen_col = &dbo->operator_fields.print_operator.phandles[0].generalized_column;
    ret_status.code = OK;

    if (start == 0){
        num_rows = print_get_num_rows(dbo);
        msg->payload = malloc(DEFAULT_QUERY_BUFFER_SIZE);
        msg->length = 0;

        if (num_rows == 0) {
            ncode = ret_status.code;
            memcpy(msg->payload, end_token1, strlen(end_token1));
            msg->length += strlen(end_token1);

            memcpy(&msg->payload[msg->length], &ncode, sizeof(int));
            msg->length += sizeof(int);

            msg->status = OK_DONE;
            return ret_status;
        }
    }
    msg->length = 0;
    memset(msg->payload, 0, DEFAULT_QUERY_BUFFER_SIZE);
    memcpy(msg->payload, start_token, strlen(start_token));
    msg->length += strlen(start_token);

    size_t limit = start + PRINT_VEC_SIZE < num_rows ? start + PRINT_VEC_SIZE : num_rows;

    if (gen_col->column_type == RESULT)
        print_values_result(msg, gen_col->column_pointer.result, start, limit);
    else if (gen_col->column_type == COLUMN)
        print_values_column(msg, gen_col->column_pointer.column, start, limit);
    else if (gen_col->column_type == SCALAR)
        print_values_scalar(msg, gen_col->column_pointer.scalar, start, limit);

    if (limit == num_rows) {
        start = 0;   
        ret_status.code = OK;
    } else {
        start = limit;
        ret_status.code = INCOMPLETE;
    }
    ncode = ret_status.code;
    memcpy(&msg->payload[msg->length], end_token, strlen(end_token));
    msg->length += strlen(end_token);

    memcpy(&msg->payload[msg->length], &ncode, sizeof(int));
    msg->length += sizeof(int);

    msg->status = OK_DONE;
    return ret_status;
}