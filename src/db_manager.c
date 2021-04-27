#include "cs165_api.h"
#include "ioutils.h"
#include "dsutil.h"
#include "ophelpers.h"
#define INIT_NTABLES 4


// In this class, there will always be only one active database at a time
Db *current_db = NULL;

/* 
 * Here you will create a table object. The Status object can be used to return
 * to the caller that there was an error in table creation
 */
Column* db_create_column(char *name, Table *table, bool sorted, Status *ret_status){
    size_t colnum;
    int slen = strlen(name);
    int nrows;
    assert(slen < MAX_SIZE_NAME);
    
    if (has_column(table, name)){
    	//log error column already exists
    	ret_status->code = ERROR;
    	return NULL;
    }
    colnum = table->create_col_count;
    assert(colnum < table->col_count);
    strcpy(table->columns[colnum].name, name);
    table->columns[colnum].isdirty = false;
    table->create_col_count++;
    
    nrows = table->table_length != 0 ? table->table_length : 32;
    table->columns[colnum].data = (int*)calloc(nrows, sizeof(int));
    assert(table->columns[colnum].data != NULL);
    table->columns[colnum].col_capacity = nrows;
    table->columns[colnum].index = NULL;
    table->columns[colnum].col_size = 0;
    table->columns[colnum].issorted = sorted;
    ret_status->code=OK;
    return &table->columns[colnum];
}

void db_initialize_index(Column* column, int idxtype){
    column->index = malloc(sizeof(ColumnIndex));
    column->index->parent = column;
    //column->index->root = NULL;
    //column->index->pv = NULL;

    switch (idxtype){
        case CLUSTER_BTREE : column->index->type = CLUSTER_BTREE; break;
        case CLUSTER_SORT : column->index->type = CLUSTER_SORT ; break;
        case UNCLUSTER_BTREE: column->index->type = UNCLUSTER_BTREE; break;
        case UNCLUSTER_SORT : column->index->type = UNCLUSTER_SORT; break;
        default: column->index->type = INDEX_NONE; break;
    }
}
/* 
 * Here you will create a table object. The Status object can be used to return
 * to the caller that there was an error in table creation
 */
Table* db_create_table(Db* db, const char* name, size_t num_columns, Status *ret_status) {
    assert(db != NULL);
    Table* table = NULL;
    if (has_table(db, name)){
    	//log error table already exists
    	ret_status->code = ERROR;
    	return NULL;
    }

    if (db->tables_size == db->tables_capacity){
	   db->tables_capacity *= 2;
	   db->tables = realloc(db->tables, db->tables_capacity*sizeof(Table));
    }
    int slen = strlen(name);
    assert(slen < MAX_SIZE_NAME);
    strcpy(db->tables[db->tables_size].name, name);

    db->tables[db->tables_size].col_count = num_columns;
    db->tables[db->tables_size].create_col_count = 0;
    db->tables[db->tables_size].table_length = 0;

    db->tables[db->tables_size].columns = (Column*)calloc(num_columns, sizeof(Column));
    assert(db->tables[db->tables_size].columns != NULL);
    table = &db->tables[db->tables_size];
    db->tables_size++;

    ret_status->code=OK;
    return table;
}
/* 
 * Similarly, this method is meant to create a database.
 * As an implementation choice, one can use the same method
 * for creating a new database and for loading a database 
 * from disk, or one can divide the two into two different
 * methods.
 */
Status db_add(const char* db_name, bool new) {
	struct Status ret_status;

    //cleans up existing db incase user wants to start new db
    if (new && current_db) db_cleanup();

    // if db already exists spit out error 
    if (current_db != NULL){
	ret_status.code = strcmp(current_db->name, db_name) == 0 ? OK : ERROR;
	return ret_status;
    }
    current_db = malloc(sizeof(Db));
    assert(current_db != NULL);
    
    int slen = strlen(db_name);
    assert(slen < MAX_SIZE_NAME);
    strcpy(current_db->name, db_name);
    
    current_db->hasindex = false;
    current_db->tables_size = 0;
    current_db->tables_capacity = INIT_NTABLES;
    current_db->tables = (Table*)calloc(current_db->tables_capacity, sizeof(Table));
    assert(current_db->tables != NULL);

    ret_status.code = OK;
    return ret_status;
}

void db_destroy(){
    delete_db_folder(current_db->name);
    delete_db_catalog();
}
/*
	cleanup internal data structures
*/
void db_cleanup(){
    Table *table = NULL;
    Column *column = NULL;

    db_destroy();

    size_t ntables = current_db->tables_size;
    for(size_t i = 0; i < ntables; i++){
	table = &(current_db->tables[i]);
	size_t ncolumns = table->col_count;
	for(size_t j = 0; j < ncolumns; j++){
           column = &(table->columns[j]);
	   free(column->data);
	   column->data = NULL;
	}
	free(table->columns);
	table->columns = NULL;
    }
    free(current_db->tables);
    current_db->tables = NULL;

    free(current_db);
    current_db = NULL;
}

bool has_table(Db *db, const char *name){
    size_t ntables = db->tables_size;

    for(size_t i = 0; i < ntables; i++){
	if (strcmp(db->tables[i].name, name) == 0)
           return true;
    }
    return false;
}

bool has_column(Table* table, const char* name){
    size_t ncolumns = table->col_count;
    for(size_t i = 0; i < ncolumns; i++){
        if (strcmp(table->columns[i].name, name) == 0)
            return true;
    }
    //log error column does not exist
    return false;
}

int handle_exists(char* handle, ClientContext* context){
    int index = -1;
    /* find the  position vector column from client context*/
    for(int i = 0; i < context->chandles_in_use; i++){
        if (strcmp(handle, context->chandle_table[i].name) == 0){
            index = i;    
            break;
        }
    }
    return index;
}

void expand_catalog_manager(ClientContext* context, int limit){
    if (context == NULL) return;
    int rem_size = context->chandle_slots - context->chandles_in_use;
    if (rem_size < limit){
        context->chandle_slots = limit < context->chandle_slots ? 2*context->chandle_slots : context->chandle_slots + limit;
        context->chandle_table = (GeneralizedColumnHandle*)realloc(context->chandle_table, context->chandle_slots*sizeof(GeneralizedColumnHandle));
    }
}

void add_base_column_catalog_manager(char* name, Column* column, ClientContext* context){
    if (context == NULL) return;

    if (context->chandle_slots == context->chandles_in_use){
        context->chandle_slots *= 2;
        context->chandle_table = (GeneralizedColumnHandle*)realloc(context->chandle_table, context->chandle_slots*sizeof(GeneralizedColumnHandle));
    }
    strcpy(context->chandle_table[context->chandles_in_use].name, name);
    context->chandle_table[context->chandles_in_use].generalized_column.column_type = COLUMN;
    context->chandle_table[context->chandles_in_use].generalized_column.column_pointer.column = column;
    context->chandles_in_use++;
}
void add_result_vector_catalog_manager(char* name, Result* result, ClientContext* context){
    if (context == NULL) return;
    int index = handle_exists(name, context);
    int pos = index != -1 ? index : context->chandles_in_use;

    assert(context->chandles_in_use < MAX_VPOOL);
    /*
    if (index == -1 && context->chandle_slots == context->chandles_in_use){
        context->chandle_slots *= 2;
        context->chandle_table = (GeneralizedColumnHandle*)realloc(context->chandle_table, context->chandle_slots*sizeof(GeneralizedColumnHandle));
    }
    */
    // store the result vector in client context 
    strcpy(context->chandle_table[pos].name, name);
    context->chandle_table[pos].generalized_column.column_type = RESULT;
    context->chandle_table[pos].generalized_column.column_pointer.result = result;
    context->chandles_in_use += (index == -1);
}
void add_position_vector_catalog_manager(char* name, PositionVector* pv, ClientContext* context){
    if (context == NULL) return;
    int index = handle_exists(name, context);
    int pos = index != -1 ? index : context->chandles_in_use;

    assert(context->chandles_in_use < MAX_VPOOL);
    /*
    if (index == -1 && context->chandle_slots == context->chandles_in_use){
        context->chandle_slots *= 2;
        context->chandle_table = (GeneralizedColumnHandle*)realloc(context->chandle_table, context->chandle_slots*sizeof(GeneralizedColumnHandle));
    }
    */
    // store the position vector in client context
    strcpy(context->chandle_table[pos].name, name);
    context->chandle_table[pos].generalized_column.column_type = POSITION;
    context->chandle_table[pos].generalized_column.column_pointer.posvec = pv;
    context->chandles_in_use += (index == -1);
}
void add_scalar_catalog_manager(char* name, Scalar* scalar, ClientContext* context){
    if (context == NULL) return;
    int index = handle_exists(name, context);
    int pos = index != -1 ? index : context->chandles_in_use;
    assert(context->chandles_in_use < MAX_VPOOL);
    /*
    if (index == -1 && context->chandle_slots == context->chandles_in_use){
        context->chandle_slots *= 2;
        context->chandle_table = (GeneralizedColumnHandle*)realloc(context->chandle_table, context->chandle_slots*sizeof(GeneralizedColumnHandle));
    }
    */
    //store the scalar in client context 
    strcpy(context->chandle_table[pos].name, name);
    context->chandle_table[pos].generalized_column.column_type = SCALAR;
    context->chandle_table[pos].generalized_column.column_pointer.scalar = scalar;
    context->chandles_in_use += (index == -1);
}
void copy_catalog(ClientContext* srvcontext, ClientContext* clientcontext){
    int nhandles = srvcontext->chandles_in_use;

    for(int i = 0; i < nhandles; i++){    
        if (srvcontext->chandle_table[i].generalized_column.column_type == COLUMN){
            if (handle_exists(srvcontext->chandle_table[i].name, clientcontext) != -1) continue;
            strcpy(clientcontext->chandle_table[i].name, srvcontext->chandle_table[i].name);
            clientcontext->chandle_table[i].generalized_column.column_type = srvcontext->chandle_table[i].generalized_column.column_type;            
            clientcontext->chandle_table[i].generalized_column.column_pointer.column = srvcontext->chandle_table[i].generalized_column.column_pointer.column;
            clientcontext->chandles_in_use++;
        }
        
    }
}

/*
void add_base_column_catalog_manager1(char* name, Column* column, ClientContext* context, GeneralizedColumnHandle** handle){
    if (context == NULL) return;

    if (context->chandle_slots == context->chandles_in_use){
        context->chandle_slots *= 2;
        *handle = (GeneralizedColumnHandle*)realloc(*handle, context->chandle_slots*sizeof(GeneralizedColumnHandle));
    }
    // store the result vector in client context 
    strcpy((*handle)[context->chandles_in_use].name, name);
    (*handle)[context->chandles_in_use].generalized_column.column_type = COLUMN;
    (*handle)[context->chandles_in_use].generalized_column.column_pointer.column = column;
    context->chandle_slots++;
}

void add_result_vector_catalog_manager1(char* name, Result* result, ClientContext* context, GeneralizedColumnHandle** handle){
    if (context == NULL) return;
    int index = handle_exists(name, context);
    int pos = index != -1 ? index : context->chandles_in_use;

    if (index == -1 && context->chandle_slots == context->chandles_in_use){
        context->chandle_slots *= 2;
        *handle = (GeneralizedColumnHandle*)realloc(*handle, context->chandle_slots*sizeof(GeneralizedColumnHandle));
    }
    // store the result vector in client context 
    strcpy((*handle)[pos].name, name);
    (*handle)[pos].generalized_column.column_type = RESULT;
    (*handle)[pos].generalized_column.column_pointer.result = result;
    context->chandle_slots += (index == -1);
}

void add_position_vector_catalog_manager1(char* name, PositionVector* pv, ClientContext* context, GeneralizedColumnHandle** handle){
    if (context == NULL) return;
    int index = handle_exists(name, context);
    int pos = index != -1 ? index : context->chandles_in_use;

    if (index == -1 && context->chandle_slots == context->chandles_in_use){
        context->chandle_slots *= 2;
        *handle = (GeneralizedColumnHandle*)realloc(*handle, context->chandle_slots*sizeof(GeneralizedColumnHandle));
    }
    // store the result vector in client context
    strcpy((*handle)[pos].name, name);
    (*handle)[pos].generalized_column.column_type = POSITION;
    (*handle)[pos].generalized_column.column_pointer.posvec = pv;
    context->chandle_slots += (index == -1);
}

void add_scalar_catalog_manager1(char* name, Scalar* scalar, ClientContext* context, GeneralizedColumnHandle** handle){
    if (context == NULL) return;
    int index = handle_exists(name, context);
    int pos = index != -1 ? index : context->chandles_in_use;

    if (index == -1 && context->chandle_slots == context->chandles_in_use){
        context->chandle_slots *= 2;
        *handle = (GeneralizedColumnHandle*)realloc(*handle, context->chandle_slots*sizeof(GeneralizedColumnHandle));
    }
    //store the result vector in client context 
    strcpy((*handle)[pos].name, name);
    (*handle)[pos].generalized_column.column_type = SCALAR;
    (*handle)[pos].generalized_column.column_pointer.scalar = scalar;
    context->chandle_slots += (index == -1);
}
void copy_handle(int nhandles, GeneralizedColumnHandle* newhandle, GeneralizedColumnHandle* oldhandle){

    for(int i = 0; i < nhandles; i++){
        strcpy(newhandle->name, oldhandle->name);
        newhandle->generalized_column.column_type = oldhandle->generalized_column.column_type;
        if (newhandle->generalized_column.column_type == COLUMN){
            newhandle->generalized_column.column_pointer.column = oldhandle->generalized_column.column_pointer.column;
            oldhandle->generalized_column.column_pointer.column = NULL;
        }
        else if (newhandle->generalized_column.column_type == RESULT){
            newhandle->generalized_column.column_pointer.result = oldhandle->generalized_column.column_pointer.result;
            oldhandle->generalized_column.column_pointer.result = NULL;
        } else if (newhandle->generalized_column.column_type == POSITION){
            newhandle->generalized_column.column_pointer.posvec = oldhandle->generalized_column.column_pointer.posvec;
            oldhandle->generalized_column.column_pointer.posvec = NULL;
        } else if (newhandle->generalized_column.column_type == SCALAR){
            newhandle->generalized_column.column_pointer.scalar = oldhandle->generalized_column.column_pointer.scalar;
            oldhandle->generalized_column.column_pointer.scalar = NULL;
        }
    }
    free(oldhandle);
}

void add_base_column_catalog_manager(char* name, Column* column, ClientContext* context){
    if (context == NULL) return;
    GeneralizedColumnHandle* newhandle = NULL;

    if (context->chandle_slots == context->chandles_in_use){
        context->chandle_slots *= 2;
        newhandle = (GeneralizedColumnHandle*)calloc(context->chandle_slots, sizeof(GeneralizedColumnHandle));
        copy_handle(context->chandles_in_use, newhandle, context->chandle_table);
        context->chandle_table = newhandle;
    }
    strcpy(context->chandle_table[context->chandles_in_use].name, name);
    context->chandle_table[context->chandles_in_use].generalized_column.column_type = COLUMN;
    context->chandle_table[context->chandles_in_use].generalized_column.column_pointer.column = column;
    context->chandles_in_use++;
}
void add_result_vector_catalog_manager(char* name, Result* result, ClientContext* context){
    if (context == NULL) return;
    GeneralizedColumnHandle* newhandle = NULL;
    int index = handle_exists(name, context);
    int pos = index != -1 ? index : context->chandles_in_use;

    if (index == -1 && context->chandle_slots == context->chandles_in_use){
        context->chandle_slots *= 2;
        newhandle = (GeneralizedColumnHandle*)calloc(context->chandle_slots, sizeof(GeneralizedColumnHandle));
        copy_handle(context->chandles_in_use, newhandle, context->chandle_table);
        context->chandle_table = newhandle;
    }
    strcpy(context->chandle_table[pos].name, name);
    context->chandle_table[pos].generalized_column.column_type = RESULT;
    context->chandle_table[pos].generalized_column.column_pointer.result = result;
    context->chandles_in_use += (index == -1);
}
void add_position_vector_catalog_manager(char* name, PositionVector* pv, ClientContext* context){
    if (context == NULL) return;
    GeneralizedColumnHandle* newhandle = NULL;
    int index = handle_exists(name, context);
    int pos = index != -1 ? index : context->chandles_in_use;

    if (index == -1 && context->chandle_slots == context->chandles_in_use){
        context->chandle_slots *= 2;
        newhandle = (GeneralizedColumnHandle*)calloc(context->chandle_slots, sizeof(GeneralizedColumnHandle));
        copy_handle(context->chandles_in_use, newhandle, context->chandle_table);
        context->chandle_table = newhandle;
    }
    strcpy(context->chandle_table[pos].name, name);
    context->chandle_table[pos].generalized_column.column_type = POSITION;
    context->chandle_table[pos].generalized_column.column_pointer.posvec = pv;
    context->chandles_in_use += (index == -1);
}
void add_scalar_catalog_manager(char* name, Scalar* scalar, ClientContext* context){
    if (context == NULL) return;
    GeneralizedColumnHandle* newhandle = NULL;
    int index = handle_exists(name, context);
    int pos = index != -1 ? index : context->chandles_in_use;

    if (index == -1 && context->chandle_slots == context->chandles_in_use){
        context->chandle_slots *= 2;
        newhandle = (GeneralizedColumnHandle*)calloc(context->chandle_slots, sizeof(GeneralizedColumnHandle));
        copy_handle(context->chandles_in_use, newhandle, context->chandle_table);
        context->chandle_table = newhandle;
    }
    strcpy(context->chandle_table[pos].name, name);
    context->chandle_table[pos].generalized_column.column_type = SCALAR;
    context->chandle_table[pos].generalized_column.column_pointer.scalar = scalar;
    context->chandles_in_use += (index == -1);
}
*/
bool cleanup_handle(char* handle, ClientContext* context){
    
    int index = handle_exists(handle, context);
    if (index == -1) return false;

    if (context->chandle_table[index].generalized_column.column_type == RESULT){
        Result * res = context->chandle_table[index].generalized_column.column_pointer.result;
        if (res != NULL){
            free(res->payload);
            res->payload = NULL;

            free(res);
            res = NULL;
        }
    } else if (context->chandle_table[index].generalized_column.column_type == POSITION){
        PositionVector * pv = context->chandle_table[index].generalized_column.column_pointer.posvec;
        if (pv != NULL){
            free(pv->positions);
            pv->positions = NULL;

            free(pv);
            pv = NULL;
        }
    }
    else if (context->chandle_table[index].generalized_column.column_type == SCALAR){
        Scalar * sc = context->chandle_table[index].generalized_column.column_pointer.scalar;
        if (sc != NULL){
            free(sc->svalue);
            sc->svalue = NULL;

            free(sc);
            sc = NULL;
        }
    }
    return true;
}

// select for greater than equal and less than
int db_select_shared_ge_less_op(DbOperator* dbo, unsigned int start, unsigned int* ntuples, unsigned int* positions){  
    unsigned bval = 0;
    unsigned int count = 0;
    int* data = (int*)dbo->operator_fields.select_operator.col_var->column_pointer.column->data;
    long icompare1 = dbo->operator_fields.select_operator.comparator.p_low;
    long icompare2 = dbo->operator_fields.select_operator.comparator.p_high;
    unsigned int num_rows = dbo->operator_fields.select_operator.col_var->column_pointer.column->col_size;

    unsigned int end = start + VEC_SIZE  < num_rows ?start + VEC_SIZE :num_rows;
    int done = end == num_rows ? true : false;
    int num_pos = *ntuples;
    for(size_t pos = start; pos < end; pos++){
        bval = data[pos] >= icompare1 && data[pos] < icompare2;
        positions[num_pos+count] |= (bval*pos);
        count += bval;
    }
    *ntuples += count;
    return done;
}

// select for less than operation
int db_select_shared_less_op(DbOperator* dbo, unsigned int start, unsigned int* ntuples, unsigned int* positions){ 
    unsigned bval = 0;
    unsigned int count = 0; 
    int* data = (int*)dbo->operator_fields.select_operator.col_var->column_pointer.column->data;
    long icompare = dbo->operator_fields.select_operator.comparator.p_high;
    unsigned int num_rows = dbo->operator_fields.select_operator.col_var->column_pointer.column->col_size;

    unsigned int end = start + VEC_SIZE  < num_rows ?start + VEC_SIZE :num_rows;
    int done = end == num_rows ? true : false;
    int num_pos = *ntuples;
    for(size_t pos = start; pos < end; pos++){
        bval = data[pos] < icompare;
        positions[num_pos+count] |= (bval*pos);
        count += bval;
    }
    *ntuples += count;
    return done;
}
// select for greater than equal operation 
int db_select_shared_ge_op(DbOperator* dbo, unsigned int start, unsigned int* ntuples, unsigned int* positions){  
    unsigned bval = 0;
    unsigned int count = 0;
    int* data = (int*)dbo->operator_fields.select_operator.col_var->column_pointer.column->data;
    long icompare = dbo->operator_fields.select_operator.comparator.p_low;
    unsigned int num_rows = dbo->operator_fields.select_operator.col_var->column_pointer.column->col_size;

    unsigned int end = start + VEC_SIZE  < num_rows ?start + VEC_SIZE :num_rows;
    int done = end == num_rows ? true : false;
    int num_pos = *ntuples;
    for(size_t pos = start; pos < end; pos++){
        bval = data[pos] >= icompare;
        positions[num_pos+count] |= (bval*pos);
        count += bval;
    }
    *ntuples += count;
    return done;
}
// select for greater operation 
int db_select_shared_greater_op(DbOperator* dbo, unsigned int start, unsigned int* ntuples, unsigned int* positions){  
    unsigned bval = 0;
    unsigned int count = 0;
    int* data = (int*)dbo->operator_fields.select_operator.col_var->column_pointer.column->data;
    long icompare = dbo->operator_fields.select_operator.comparator.p_low;
    unsigned int num_rows = dbo->operator_fields.select_operator.col_var->column_pointer.column->col_size;

    unsigned int end = start + VEC_SIZE  < num_rows ?start + VEC_SIZE :num_rows;
    int done = end == num_rows ? true : false;
    int num_pos = *ntuples;
    for(size_t pos = start; pos < end; pos++){
        bval = data[pos] > icompare;
        positions[num_pos+count] |= (bval*pos);
        count += bval;
    }
    *ntuples += count;
    return done;
}
// select for less than equal operation
int db_select_shared_le_op(DbOperator* dbo,unsigned int start, unsigned int* ntuples, unsigned int* positions){  
    unsigned bval = 0;
    unsigned int count = 0;
    int* data = (int*)dbo->operator_fields.select_operator.col_var->column_pointer.column->data;
    long icompare = dbo->operator_fields.select_operator.comparator.p_high;
    unsigned int num_rows = dbo->operator_fields.select_operator.col_var->column_pointer.column->col_size;

    unsigned int end = start + VEC_SIZE  < num_rows ?start + VEC_SIZE :num_rows;
    int done = end == num_rows ? true : false;
    int num_pos = *ntuples;
    for(size_t pos = start; pos < end; pos++){
        bval = data[pos] <= icompare;
        positions[num_pos+count] |= (bval*pos);
        count += bval;
    }   
    *ntuples += count;
    return done;
}

void add_to_task_array(DbOperator* dbo){
    Task* t = malloc(sizeof(Task));  
    t->dbo = dbo;

    switch(dbo->operator_fields.select_operator.select_type){
        case SEL_LESS_OP:
            t->func = db_select_shared_less_op;
            break;
        case SEL_GE_OP:
            t->func = db_select_shared_ge_op;
            break;
        case SEL_GE_LESS_OP:
            t->func = db_select_shared_ge_less_op;
            break;
        default:
            break;
    }
    t->ntuples = 0;
    t->nallocsz = dbo->operator_fields.select_operator.col_var->column_pointer.column->col_size;
    t->positions = calloc(t->nallocsz, sizeof(unsigned int));
    tarray_push(t);
}

