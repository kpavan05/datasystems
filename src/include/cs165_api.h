

/* BREAK APART THIS API (TODO MYSELF) */
/* PLEASE UPPERCASE ALL THE STUCTS */

/*
Copyright (c) 2015 Harvard University - Data Systems Laboratory (DASLab)
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef CS165_H
#define CS165_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <limits.h>
#include "message.h"
#include "btree.h"

#define VEC_SIZE 3200
#define MAX_VPOOL 128
#define MAX_PARTITIONS 64
#define MAX_BASE_COL 32
#define PRINT_VEC_SIZE 256

#define SMALL_BUF_SIZE 128
#define DEFAULT_QUERY_BUFFER_SIZE 2048

// Limits the size of a name in our database to 64 characters
#define MAX_SIZE_NAME 64
#define MAX_DATA_LINE 256
#define MAX_LINE_SIZE 512
#define INVALID_VAL INT_MIN

/**
 * EXTRA
 * DataType
 * Flag to mark what type of data is held in the struct.
 * You can support additional types by including this enum and using void*
 * in place of int* in db_operator simliar to the way IndexType supports
 * additional types.
 **/

typedef enum DataType {
     BIT,
     INT,
     LONG,
     DOUBLE,
     FLOAT
} DataType;

struct Comparator;
struct ColumnIndex;
struct PositionVector;
//Defines index type.
typedef enum IndexType {
    CLUSTER_BTREE,
    CLUSTER_SORT,
    UNCLUSTER_BTREE,
    UNCLUSTER_SORT,
    INDEX_NONE
} IndexType;



typedef struct Column {
    char name[MAX_SIZE_NAME];
    char full_name[MAX_SIZE_NAME];
    int* data;
    // You will implement column indexes later. 
    //void* index;
    struct ColumnIndex *index;
    bool clustered;
    bool isdirty;
    bool issorted;
    size_t col_size;
    size_t col_capacity;
} Column;



typedef struct ColumnIndex {
    IndexType type;
    Column* parent;
    union{
        struct {
            //int* copy_data;
            Node* root; 
        } treeidx;
        struct {
            struct PositionVector* pv;
            int* copy_data;
        } sortidx;  
    }idxdata;
} ColumnIndex;

/**
 * table
 * Defines a table structure, which is composed of multiple columns.
 * We do not require you to dynamically manage the size of your tables,
 * although you are free to append to the struct if you would like to (i.e.,
 * include a size_t table_size).
 * name, the name associated with the table. table names must be unique
 *     within a database, but tables from different databases can have the same
 *     name.
 * - col_count, the number of columns in the table
 * - col,umns this is the pointer to an array of columns contained in the table.
 * - table_length, the size of the columns in the table.
 **/

typedef struct Table {
    char name [MAX_SIZE_NAME];
    Column *columns;
    size_t col_count;
    size_t table_length;
    size_t create_col_count;
} Table;

/**
 * db
 * Defines a database structure, which is composed of multiple tables.
 * - name: the name of the associated database.
 * - tables: the pointer to the array of tables contained in the db.
 * - tables_size: the size of the array holding table objects
 * - tables_capacity: the amount of pointers that can be held in the currently allocated memory slot
 **/

typedef struct Db {
    char name[MAX_SIZE_NAME]; 
    Table *tables;
    size_t tables_size;
    size_t tables_capacity;
    bool hasindex;
} Db;

/**
 * Error codes used to indicate the outcome of an API call
 **/
typedef enum StatusCode {
  /* The operation completed successfully */
  OK,
  /* There was an error with the call. */
  ERROR,
  INCOMPLETE
} StatusCode;


// status declares an error code and associated message
typedef struct Status {
    StatusCode code;
    char* error_message;
} Status;

// Defines a comparator flag between two values.
typedef enum ComparatorType {
    NONE = 0,
    NO_COMPARISON = 1,
    LESS_THAN = 2,
    GREATER_THAN = 3,
    EQUAL = 4,
    LESS_THAN_OR_EQUAL = 5,
    GREATER_THAN_OR_EQUAL = 6,
} ComparatorType;

// Defines a scalar operator between two values.
typedef enum ScalarOpType {
    AVG = 1,
    SUM = 2,
    MIN = 3,
    MAX = 4
} ScalarOpType;

// Defines a arithmetic operator type.
typedef enum ArithmeticOpType {
    ADD,
    SUB
} ArithmeticOpType;

//Defines create operation type.
typedef enum CreateOpType {
    CREATE_DB = 0,
    CREATE_TABLE = 1,
    CREATE_COL =2,
    CREATE_IDX = 3,
} CreateOpType;

/*
 * Declares the type of scalar value
*/
typedef struct Scalar{
    DataType data_type;
    void *svalue;
}Scalar;
/*
 * Declares the type of a position vector, 
 which includes the number of tuples in the result, and a pointer to the positions data
 */
typedef struct PositionVector {
    size_t num_tuples;
    //DataType data_type;
    unsigned int *positions;
} PositionVector;
/*
 * Declares the type of a result column, 
 which includes the number of tuples in the result, the data type of the result, and a pointer to the result data
*/
typedef struct Result {
    size_t num_tuples;
    DataType data_type;
    //PositionVector* posvec;
    Column *base;
    void *payload;
} Result;
/*
 * an enum which allows us to differentiate between columns and results
 */
typedef enum GeneralizedColumnType {
    RESULT,
    POSITION,
    SCALAR,
    COLUMN
} GeneralizedColumnType;
/*
 * a union type holding either a column or a result struct
 */
typedef union GeneralizedColumnPointer {
    Result* result;   
    Column* column;
    PositionVector* posvec;
    Scalar* scalar;
} GeneralizedColumnPointer;

/*
 * unifying type holding either a column or a result
 */
typedef struct GeneralizedColumn {
    GeneralizedColumnType column_type;
    GeneralizedColumnPointer column_pointer;
} GeneralizedColumn;
/*
 * used to refer to a column in our client context
 */

typedef struct GeneralizedColumnHandle {
    char name[MAX_SIZE_NAME];
    GeneralizedColumn generalized_column;
} GeneralizedColumnHandle;
/*
 * holds the information necessary to refer to generalized columns (results or columns)
 */
typedef struct ClientContext {
    GeneralizedColumnHandle* chandle_table;
    int chandles_in_use;
    int chandle_slots;
} ClientContext;

/**
 * comparator
 * A comparator defines a comparison operation over a column. 
 **/
typedef struct Comparator {
    long int p_low; // used in equality and ranges.
    long int p_high; // used in range compares. 
    GeneralizedColumn* gen_col;
    ComparatorType type1;
    ComparatorType type2;
    char* handle;
} Comparator;
typedef enum JoinOpType {
    NESTED_LOOP,
    HASH_JOIN,
    SORT_MERGE,
    GRACE_JOIN
} JoinOpType;
/*
 * tells the databaase what type of operator this is
 */
typedef enum OperatorType {
    CREATE,
    INSERT,
    SELECT,
    FETCH,
    OPEN,
    LOAD,
    JOIN,
    PRINT,
    CLOSE,
    UPDATE,
    DELETE,
    SCALAR_OP,
    BATCH_EXECUTE,
    ARITHMETIC_OP
} OperatorType;
/*
 * necessary fields for insertion
*/
typedef struct InsertOperator {
    Table* table;
    int* values;
} InsertOperator;

typedef enum SelectOpType{
    SEL_GE_LESS_OP,
    SEL_GE_OP,
    SEL_LESS_OP,
    SEL_NONE
}SelectOpType;
/*
 * necessary fields for selection
*/
typedef struct SelectOperator {
    char res_name[MAX_SIZE_NAME];
    GeneralizedColumn* col_var;
    GeneralizedColumn* pos_vec;
    Comparator comparator;
    SelectOpType select_type;
} SelectOperator;
/*
 * necessary fields for fetch
*/
typedef struct FetchOperator {
    char res_name[MAX_SIZE_NAME];
    GeneralizedColumn* col_var;
    GeneralizedColumn* vec_pos;    
} FetchOperator;
/*
 * necessary fields for scalar operators
*/
typedef struct ScalarOperator {
    char res_name[MAX_SIZE_NAME];
    ScalarOpType op_type;
    GeneralizedColumn* col_var;   
} ScalarOperator;
/*
 * necessary fields for arithmetic operators
*/
typedef struct ArithmeticOperator {
    char res_name[MAX_SIZE_NAME];
    ArithmeticOpType op_type;
    GeneralizedColumn* col_var1;
    GeneralizedColumn* col_var2;  
} ArithmeticOperator;
/*
 * necessary fields for open
*/
typedef struct OpenOperator {
    char* db_name;
} OpenOperator;
/*
 * necessary fields for load
*/
typedef struct LoadOperator {
    Table* table;
    int rstate;
    char* data;
    long* ptr_arr;
} LoadOperator;
/*
 * necessary fields for update
*/
typedef struct UpdateOperator {
    Table* table;
    Column* col_var;
    GeneralizedColumn* pos_vec; 
    int update_value;   
} UpdateOperator;
/*
 * necessary fields for delete
*/
typedef struct DeleteOperator {
    Table* table;
    GeneralizedColumn* pos_vec;    
} DeleteOperator;
/*
 * necessary fields for load
*/
typedef struct JoinOperator {
    char res_name1[MAX_SIZE_NAME];
    char res_name2[MAX_SIZE_NAME];
    GeneralizedColumn* sel1;
    GeneralizedColumn* val1;
    GeneralizedColumn* sel2;
    GeneralizedColumn* val2;
    JoinOpType optype;
}JoinOperator;
/*
 * necessary fields for print
*/
typedef struct PrintOperator {
    int nhandles;
    GeneralizedColumnHandle *phandles;
} PrintOperator;
/*
 * necessary fields for creation
*/
typedef struct CreateOperator{
    CreateOpType op_type;
    int num_columns;
    char db_name[MAX_SIZE_NAME];
    char tbl_name[MAX_SIZE_NAME];
    char col_name[MAX_SIZE_NAME];
    char full_name[MAX_SIZE_NAME];
    IndexType idx_type;
} CreateOperator;
/*
 * union type holding the fields of any operator
 */
typedef union OperatorFields {
    CreateOperator create_operator;
    InsertOperator insert_operator;
    OpenOperator open_operator;
    LoadOperator load_operator;
    SelectOperator select_operator;
    FetchOperator fetch_operator;
    ScalarOperator scalar_operator;
    PrintOperator print_operator;
    JoinOperator join_operator;
    UpdateOperator update_operator;
    DeleteOperator delete_operator;
    ArithmeticOperator arithmetic_operator;
} OperatorFields;
/*
 * DbOperator holds the following fields:
 * type: the type of operator to perform (i.e. insert, select, ...)
 * operator fields: the fields of the operator in question
 * client_fd: the file descriptor of the client that this operator will return to
 * context: the context of the operator in question. This context holds the local results of the client in question.
 */
typedef struct DbOperator {
    OperatorType type;
    OperatorFields operator_fields;
    int client_fd;
    ClientContext* context;
    bool isbatched;
} DbOperator;

extern Db *current_db;

Status db_startup(ClientContext* context);

/**
 * sync_db(db)
 * Saves the current status of the database to disk.
 *
 * db       : the database to sync.
 * returns  : the status of the operation.
 **/
Status db_sync(Db* db);
Status db_add(const char* db_name, bool new);

Table* db_create_table(Db* db, const char* name, size_t num_columns, Status *status);
Column* db_create_column(char *name, Table *table, bool sorted, Status *ret_status);
Status db_create_index(Table* table, Column* column);
void db_initialize_index(Column* column, int idxtype);
void db_restore_index();

Status shutdown_server();
Status db_shutdown(Db* db);

Status db_fetch(DbOperator *dbo);
Status db_load(DbOperator* query);
Status db_join(DbOperator *query);
Status db_create(DbOperator *query);
Status db_insert(DbOperator* query);
Status db_select(DbOperator* query);
Status db_update(DbOperator* query);
Status db_delete(DbOperator* query);
Status db_scalar_op(DbOperator* query);
Status db_arithmetic_op(DbOperator* dbo);
Status db_batch_execute();
void db_cleanup();

Status execute_DbOperator(DbOperator* query, message* msg);
void db_operator_free(DbOperator* query, int* is_srv_running);
void db_clear_client_context(ClientContext* context);

bool has_table(Db *db, const char *name);
bool has_column(Table *table, const char *name);


bool cleanup_handle(char* handle, ClientContext* context);
//void expand_catalog_manager(ClientContext* context, int limit);
void copy_catalog(ClientContext* srvcontext, ClientContext* clientcontext);
void add_scalar_catalog_manager(char* name, Scalar* scalar, ClientContext* context);
void add_base_column_catalog_manager(char* name, Column* column, ClientContext* context);
void add_result_vector_catalog_manager(char* name, Result* result, ClientContext* context);
void add_position_vector_catalog_manager(char* name, PositionVector* pv, ClientContext* context);
void add_to_task_array(DbOperator* dbo);
#endif /* CS165_H */

