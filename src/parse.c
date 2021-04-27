/* 
 * This file contains methods necessary to parse input from the client.
 * Mostly, functions in parse.c will take in string input and map these
 * strings into database operators. This will require checking that the
 * input from the client is in the correct format and maps to a valid
 * database operator.
 */

#define _BSD_SOURCE
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include "cs165_api.h"
#include "parse.h"
#include "utils.h"
 #include "dsutil.h"
#include "client_context.h"


void parse_remove_parens(char* query_command, message* send_message){
    if (strncmp(query_command, "(", 1) == 0)
        query_command[0] = ' ';
    else
        send_message->status = UNKNOWN_COMMAND;

    int last_char = strlen(query_command) - 1;
    if (last_char < 0 || query_command[last_char] != ')') {
        send_message->status = INCORRECT_FORMAT;
    }
    query_command[last_char] = '\0';
    query_command = trim_whitespace(query_command);
}
/*
  read the select low limit and hight limit of range query  
*/
long int parse_select_range(char** tokenizer_copy){
    char* cval;
    long int ival;
    char *endptr;

    if((cval = strsep(tokenizer_copy, ",")) == NULL || strcmp(cval, "null") == 0){
       return INVALID_VAL;
    }
    ival = strtol(cval, &endptr, 10);
    return ival;
}
/*
 set comparator type for select operator
*/
void parse_set_comparator_type(DbOperator *dbo){
    if (dbo == NULL) return;
    long int lo = dbo->operator_fields.select_operator.comparator.p_low;
    long int hi = dbo->operator_fields.select_operator.comparator.p_high;
    dbo->operator_fields.select_operator.comparator.type1 = lo != INVALID_VAL ? GREATER_THAN_OR_EQUAL :  NONE;
    dbo->operator_fields.select_operator.comparator.type2 = hi != INVALID_VAL ? LESS_THAN : NONE;

    dbo->operator_fields.select_operator.select_type = (lo != INVALID_VAL && hi != INVALID_VAL) ? SEL_GE_LESS_OP : lo != INVALID_VAL ? SEL_GE_OP : hi != INVALID_VAL ? SEL_LESS_OP :SEL_NONE;
   
}
/**
 * This method takes in a string representing the arguments to create a column
 * It parses those arguments, checks that they are valid, and creates a column.
 **/
message_status parse_create_col(char* create_arguments, DbOperator* dbo) {
    message_status status = OK_DONE;
    int token_status = 0;

    char** create_arguments_index = &create_arguments;
    char* col_name = next_token(create_arguments_index, COMMA, &token_status);
    char* db_tbl_name = next_token(create_arguments_index, COMMA, &token_status);

    // not enough arguments
    if (token_status != 0) {
        return INCORRECT_FORMAT;
    }
    // Get the table name free of quotation marks
    col_name = trim_quotes(col_name);
    db_tbl_name = trim_quotes(db_tbl_name);

    strcpy(dbo->operator_fields.create_operator.tbl_name, db_tbl_name);
    strcpy(dbo->operator_fields.create_operator.col_name, col_name);
    sprintf(dbo->operator_fields.create_operator.full_name,"%s.%s", db_tbl_name,col_name); 
    return status;
}
/**
 * This method takes in a string representing the arguments to create a table.
 * It parses those arguments, checks that they are valid, and creates a table.
 **/
message_status parse_create_tbl(char* create_arguments, DbOperator* dbo) {
    message_status status = OK_DONE;

    int token_status = 0;
    char** create_arguments_index = &create_arguments;
    char* table_name = next_token(create_arguments_index, COMMA, &token_status);
    char* db_name = next_token(create_arguments_index, COMMA, &token_status);
    char* col_cnt = next_token(create_arguments_index, COMMA, &token_status);

    // not enough arguments
    if (token_status != 0) {
        return INCORRECT_FORMAT;
    }
    // Get the table name free of quotation marks
    table_name = trim_quotes(table_name);
    db_name = trim_quotes(db_name);
   
    // check that the database argument is the current active database
    if (strcmp(current_db->name, db_name) != 0) {
        cs165_log(stdout, "query unsupported. Bad db name");
        return QUERY_UNSUPPORTED;
    }

    // turn the string column count into an integer, and check that the input is valid.
    int column_cnt = atoi(col_cnt);
    if (column_cnt < 1) {
        return INCORRECT_FORMAT;
    }
    strcpy(dbo->operator_fields.create_operator.db_name, db_name);
    strcpy(dbo->operator_fields.create_operator.tbl_name, table_name);
    dbo->operator_fields.create_operator.num_columns = column_cnt;
    return status;
}
/**
 * This method takes in a string representing the arguments to create a database.
 * It parses those arguments, checks that they are valid, and creates a database.
 **/
message_status parse_create_db(char* create_arguments, DbOperator* dbo) {
    char *token;
    token = strsep(&create_arguments, ",");
    // not enough arguments if token is NULL
    if (token == NULL) {
        return INCORRECT_FORMAT;                    
    } else {
        // create the database with given name
        char* db_name = token;
        // trim quotes and check for finishing parenthesis.
        db_name = trim_quotes(db_name);
        token = strsep(&create_arguments, ",");
        if (token != NULL) {
            return INCORRECT_FORMAT;
        }
        strcpy(dbo->operator_fields.create_operator.db_name, db_name);
        return OK_DONE;
    }
}
/**
 * This method takes in a string representing the arguments to create an index.
 * It parses those arguments, checks that they are valid, and creates an index file.
 **/
message_status parse_create_idx(char* create_arguments, DbOperator* dbo) {

    int token_status = 0;
    char** create_arguments_index = &create_arguments;

    char* col_name = next_token(create_arguments_index, COMMA, &token_status);
    if (token_status != 0) return INCORRECT_FORMAT;   
    //if (lookup_generalized_column(col_name, dbo->context) == NULL) return OBJECT_NOT_FOUND;

    strcpy(dbo->operator_fields.create_operator.col_name, col_name);

    char* idx_type = next_token(create_arguments_index, COMMA, &token_status);
    if (token_status != 0) return INCORRECT_FORMAT;
   
    char* is_clustered = next_token(create_arguments_index, COMMA, &token_status);
    if (token_status != 0) return INCORRECT_FORMAT;
    
    if (strcmp(idx_type, "btree") == 0){
        dbo->operator_fields.create_operator.idx_type = strcmp(is_clustered,"clustered") == 0 ? CLUSTER_BTREE : UNCLUSTER_BTREE;
    } else if (strcmp(idx_type, "sorted") == 0){
        dbo->operator_fields.create_operator.idx_type = strcmp(is_clustered,"clustered") == 0 ? CLUSTER_SORT: UNCLUSTER_SORT;
    }
    return OK_DONE;
}
/**
 * parse_create parses a create statement and then passes the necessary arguments off to the next function
 **/
DbOperator* parse_create(char* create_arguments, message *send_message) {
    //message_status mes_status = OK_DONE;
    int token_status = 0;
    char *token;
    char *tokenizer_copy, *to_free;

    send_message->status = OK_DONE;
    parse_remove_parens(create_arguments, send_message);
    if (send_message->status != OK_DONE) return NULL;

    // Since strsep destroys input, we create a copy of our input. 
    tokenizer_copy = to_free = malloc((strlen(create_arguments)+1) * sizeof(char));
    
    strcpy(tokenizer_copy, create_arguments);
    // make load operator. 
    DbOperator* dbo = malloc(sizeof(DbOperator));
    dbo->type = CREATE;
    memset(dbo->operator_fields.create_operator.db_name, 0, MAX_SIZE_NAME);
    memset(dbo->operator_fields.create_operator.tbl_name, 0, MAX_SIZE_NAME);
    memset(dbo->operator_fields.create_operator.col_name, 0, MAX_SIZE_NAME);

    token = next_token(&tokenizer_copy, COMMA, &token_status);
    if (strcmp(token, "db") == 0) {
        send_message->status = parse_create_db(tokenizer_copy, dbo);
        dbo->operator_fields.create_operator.op_type = CREATE_DB;
    } else if (strcmp(token, "tbl") == 0) {
        send_message->status = parse_create_tbl(tokenizer_copy, dbo);
        dbo->operator_fields.create_operator.op_type = CREATE_TABLE;
    } else if (strcmp(token, "col") == 0) {
        send_message->status = parse_create_col(tokenizer_copy, dbo);
        dbo->operator_fields.create_operator.op_type = CREATE_COL;
    } else if (strcmp(token, "idx") == 0) {
        send_message->status = parse_create_idx(tokenizer_copy, dbo);
        dbo->operator_fields.create_operator.op_type = CREATE_IDX;
    }
    else {
        send_message->status = UNKNOWN_COMMAND;
    }

    if (send_message->status != OK_DONE){
        free(dbo);
        dbo = NULL;
    }
    free(to_free);
    return dbo;
}
/**
 * parse_load parses a load statement and then passes the necessary arguments off to the next function
 **/
DbOperator* parse_load(char* query_command, message* send_message) {
    int token_status = 0;
    char* token = NULL;

    parse_remove_parens(query_command, send_message);
    if (send_message->status == UNKNOWN_COMMAND || send_message->status == INCORRECT_FORMAT) return NULL;
    
    char** command_index = &query_command;
    Table* table = NULL;
    
    char* hdr= next_token(command_index, AT, &token_status);
    if (token_status != 0) {
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }
    char* data = next_token(command_index, AT, &token_status);
    if (token_status != 0) {
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }
    char* rstate = next_token(command_index, AT, &token_status);
    if (token_status != 0) {
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }
    
    rstate = rstate + 3;
    data = data + 3;

    
    // make load operator. 
    DbOperator* dbo = malloc(sizeof(DbOperator));
    dbo->type = LOAD;
    
    dbo->operator_fields.load_operator.rstate = atoi(rstate);
    dbo->operator_fields.load_operator.data = malloc(strlen(data) + 1);
    strcpy(dbo->operator_fields.load_operator.data, data);

    int count = 0;
    int ncolumns = find_num_tokens(hdr, ',');
    dbo->operator_fields.load_operator.ptr_arr = malloc(ncolumns*sizeof(long));

    while ((token = strsep(&hdr, ",")) != NULL) {
        Column *column = lookup_column(token, &table);
        if (column == NULL) {
            send_message->status = OBJECT_NOT_FOUND;
            return NULL;
        }
        dbo->operator_fields.load_operator.ptr_arr[count++] = (long)(column);
    }   
    dbo->operator_fields.load_operator.table = table;
    send_message->status = OK_DONE;
    return dbo;    
}
/**
 * parse_insert reads in the arguments for a create statement and 
 * then passes these arguments to a database function to insert a row.
 **/
DbOperator* parse_insert(char* query_command, message* send_message) {
    unsigned int columns_inserted = 0;
    int token_status = 0;
    char* token = NULL;

    parse_remove_parens(query_command, send_message);
    if (send_message->status == UNKNOWN_COMMAND || send_message->status == INCORRECT_FORMAT) return NULL;

    char** command_index = &query_command;
    // parse table input
    char* table_name = next_token(command_index, COMMA, &token_status);
    if (token_status != 0) {
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }
    // lookup the table and make sure it exists. 
    Table* insert_table = lookup_table(table_name);
    if (insert_table == NULL) {
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
    // make insert operator. 
    DbOperator* dbo = malloc(sizeof(DbOperator));
    dbo->type = INSERT;
    dbo->operator_fields.insert_operator.table = insert_table;
    dbo->operator_fields.insert_operator.values = malloc(sizeof(int) * insert_table->col_count);
    // parse inputs until we reach the end. Turn each given string into an integer. 
    while ((token = strsep(command_index, ",")) != NULL) {
        int insert_val = atoi(token);
        dbo->operator_fields.insert_operator.values[columns_inserted] = insert_val;
        columns_inserted++;
    }
    // check that we received the correct number of input values
    if (columns_inserted != insert_table->col_count) {
        send_message->status = INCORRECT_FORMAT;
        free (dbo);
        return NULL;
    } 
    send_message->status = OK_DONE;
    return dbo;  
}
/**
 * parse_select reads in the arguments for a select statement and 
 * then passes these arguments to a database function to select tuples.
 **/
DbOperator* parse_select(char* query_command, char* handle, ClientContext* context, message* send_message) {
    int token_status = 0;
    GeneralizedColumn* pos_vec = NULL;
    GeneralizedColumn* sel_col = NULL;

    parse_remove_parens(query_command, send_message);
    if (send_message->status == UNKNOWN_COMMAND || send_message->status == INCORRECT_FORMAT) return NULL;
    if (handle == NULL || handle[0] == '\0'){
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }
    char** command_index = &query_command;

    int ntokens = find_num_tokens(*command_index, ',');
    if (ntokens == 4){
        char *token = next_token(command_index, COMMA, &token_status);
        if (token_status != 0 || context == NULL) {
            send_message->status = INCORRECT_FORMAT;
            return NULL;
        }
        if ((pos_vec = lookup_generalized_column(token, context)) == NULL){
            send_message->status = OBJECT_NOT_FOUND;
            return NULL;
        }
    } 

    char* col_name = next_token(command_index, COMMA, &token_status);
    if (token_status != 0) {
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }
    if ((sel_col = lookup_generalized_column(col_name, context)) == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
    
    // make select operator. 
    DbOperator* dbo = malloc(sizeof(DbOperator));
    dbo->type = SELECT;
    dbo->operator_fields.select_operator.col_var = sel_col;
    dbo->operator_fields.select_operator.pos_vec = pos_vec;

    dbo->operator_fields.select_operator.comparator.p_low = parse_select_range(command_index);
    dbo->operator_fields.select_operator.comparator.p_high = parse_select_range(command_index);
    dbo->operator_fields.select_operator.comparator.handle = NULL;
    dbo->operator_fields.select_operator.comparator.gen_col = NULL;
    parse_set_comparator_type(dbo);
   
    send_message->status = OK_DONE;
    if (handle[0] != '\0'){
        cleanup_handle(handle, context);
        strcpy(dbo->operator_fields.select_operator.res_name, handle); 
    }
    return dbo;
}
/**
 * parse_select reads in the arguments for a select statement and 
 * then passes these arguments to a database function to select tuples.
 **/
DbOperator* parse_fetch(char* query_command, char* handle, ClientContext* context, message* send_message) {
    int token_status = 0;
    GeneralizedColumn* fetch_col = NULL;
    GeneralizedColumn* vec_pos = NULL;

    if (context == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }

    parse_remove_parens(query_command, send_message);
    if (send_message->status == UNKNOWN_COMMAND || send_message->status == INCORRECT_FORMAT) return NULL;
    if (handle == NULL || handle[0] == '\0'){
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }
    if (context == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
    char** command_index = &query_command;
    char* col_name = next_token(command_index, COMMA, &token_status);
    if (token_status != 0) {
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }
    char* posv_name = next_token(command_index, COMMA, &token_status);
    if (token_status != 0) {
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }

    if ((fetch_col = lookup_generalized_column(col_name, context)) == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
        
    if ((vec_pos = lookup_generalized_column(posv_name, context)) == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
    // make select operator. 
    DbOperator* dbo = malloc(sizeof(DbOperator));
    dbo->type = FETCH;
    dbo->operator_fields.fetch_operator.col_var = fetch_col;
    dbo->operator_fields.fetch_operator.vec_pos = vec_pos;
    
    send_message->status = OK_DONE;
    if (handle[0] != '\0'){ 
        cleanup_handle(handle, context);   
        strcpy(dbo->operator_fields.fetch_operator.res_name, handle);    
    }
    return dbo;
}
/*
    parse average, min, max
*/
DbOperator* parse_scalar_op(char* query_command, char* handle, ClientContext* context, message* send_message){
    int token_status = 0;
    GeneralizedColumn* col_var = NULL;

    if (context == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }

    parse_remove_parens(query_command, send_message);
    if (send_message->status == UNKNOWN_COMMAND || send_message->status == INCORRECT_FORMAT) return NULL;
    if (handle == NULL || handle[0] == '\0'){
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }
    
    char** command_index = &query_command;
    char* col_name = next_token(command_index, COMMA, &token_status);
    if (token_status != 0) {
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }
    if ((col_var = lookup_generalized_column(col_name, context)) == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }

    DbOperator* dbo = malloc(sizeof(DbOperator));
    dbo->type = SCALAR_OP;
    dbo->operator_fields.scalar_operator.col_var = col_var;

    send_message->status = OK_DONE;
    if (handle[0] != '\0'){
        cleanup_handle(handle, context);     
        strcpy(dbo->operator_fields.scalar_operator.res_name, handle); 
    } 
    return dbo;
}
/*
    parse add or sub commands
*/
DbOperator* parse_arithmetic_op(char* query_command, char* handle, ClientContext* context, message* send_message){
    int token_status = 0;
    GeneralizedColumn* col_var1 = NULL, *col_var2 = NULL;

    if (context == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
    parse_remove_parens(query_command, send_message);
    if (send_message->status == UNKNOWN_COMMAND || send_message->status == INCORRECT_FORMAT) return NULL;
    if (handle == NULL || handle[0] == '\0'){
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }

    char** command_index = &query_command;
    char* col_names[2];

    col_names[0] = next_token(command_index, COMMA, &token_status);
    if (token_status != 0) {
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }
    col_names[1]= next_token(command_index, COMMA, &token_status);
    if (token_status != 0) {
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }
    col_var1 = lookup_generalized_column(col_names[0], context);
    col_var2 = lookup_generalized_column(col_names[1], context);
    if (col_var1 == NULL || col_var2 == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
    
    DbOperator* dbo = malloc(sizeof(DbOperator));
    dbo->type = ARITHMETIC_OP;
    dbo->operator_fields.arithmetic_operator.col_var1 = col_var1;
    dbo->operator_fields.arithmetic_operator.col_var2 = col_var2;

    send_message->status = OK_DONE;
    if (handle[0] != '\0'){
        cleanup_handle(handle, context);   
        strcpy(dbo->operator_fields.arithmetic_operator.res_name, handle);        
    }
    return dbo;
}
/**
 * parse_print reads in the arguments for print statement, identifies the columns and 
 * then passes these arguments to a database function to print the values of columns.
 **/
DbOperator* parse_print(char* query_command, ClientContext* context, message* send_message) {
    int token_status = 0;
    char* token;
    parse_remove_parens(query_command, send_message);
    if (send_message->status == UNKNOWN_COMMAND || send_message->status == INCORRECT_FORMAT) return NULL;
   
    if (context == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
    char** command_index = &query_command;
    int ntokens = find_num_tokens(*command_index, ',');
    int nhandles = 0;
    bool bfound_col = false;
    // make select operator. 
    DbOperator* dbo = malloc(sizeof(DbOperator));
    dbo->type = PRINT;
    dbo->operator_fields.print_operator.nhandles = 0;
    dbo->operator_fields.print_operator.phandles = calloc(ntokens, sizeof(GeneralizedColumnHandle));
    
    while((token = next_token(command_index, COMMA, &token_status)) != NULL && token[0] != '\0' ){
        bfound_col = false;
        for(int i = 0; i < context->chandles_in_use; i++){
            if (strcmp(token, context->chandle_table[i].name) == 0){
                dbo->operator_fields.print_operator.phandles[nhandles++] = context->chandle_table[i];
                dbo->operator_fields.print_operator.nhandles++;
                bfound_col = true;
            }
        }
        if (!bfound_col){
            send_message->status = OBJECT_NOT_FOUND;
            free(dbo->operator_fields.print_operator.phandles);
            free(dbo);
            return NULL;
        }
    }
    send_message->status = OK_DONE;
    return dbo;
}
/*
    parse join command
*/
DbOperator* parse_join(char* query_command, char* handle, ClientContext* context, message* send_message) {
    int token_status = 0;
    char* token;
    char* jtype;
    GeneralizedColumn *s1 = NULL, *v1 = NULL;
    GeneralizedColumn *s2 = NULL, *v2 = NULL;

    if (context == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
    parse_remove_parens(query_command, send_message);
    if (send_message->status == UNKNOWN_COMMAND || send_message->status == INCORRECT_FORMAT) return NULL;
    if (handle == NULL || handle[0] == '\0'){
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }

    char** command_index = &query_command;
    char**  res_index = &handle;

    token = next_token(command_index, COMMA, &token_status);
    v1 = lookup_generalized_column(token, context);
    token = next_token(command_index, COMMA, &token_status);
    s1 = lookup_generalized_column(token, context);
    token = next_token(command_index, COMMA, &token_status);
    v2 = lookup_generalized_column(token, context);
    token = next_token(command_index, COMMA, &token_status);
    s2 = lookup_generalized_column(token, context);
    if (s1 == NULL || v1 == NULL ||  s2 == NULL ||v2 == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
    jtype = next_token(command_index, COMMA, &token_status);

    DbOperator* dbo = malloc(sizeof(DbOperator));
    dbo->type = JOIN;
    dbo->operator_fields.join_operator.sel1 = s1;
    dbo->operator_fields.join_operator.val1 = v1;
    dbo->operator_fields.join_operator.sel2 = s2;
    dbo->operator_fields.join_operator.val2 = v2;

    if (strcmp(jtype, "hash") == 0 ){
        dbo->operator_fields.join_operator.optype = HASH_JOIN;
    } else if (strcmp(jtype, "nested-loop") == 0){
        dbo->operator_fields.join_operator.optype =  NESTED_LOOP;
    } else if (strcmp(jtype, "sort-merge") == 0){
        dbo->operator_fields.join_operator.optype = SORT_MERGE;
    } else if (strcmp(jtype, "grace-join") == 0){
        dbo->operator_fields.join_operator.optype = GRACE_JOIN;
    }
    
    send_message->status = OK_DONE;
    if (handle[0] != '\0'){
        token = next_token(res_index, COMMA, &token_status);
        cleanup_handle(token, context);     
        strcpy(dbo->operator_fields.join_operator.res_name1, token);

        token = next_token(res_index, COMMA, &token_status);
        cleanup_handle(token, context);     
        strcpy(dbo->operator_fields.join_operator.res_name2, token);
    }
    return dbo;
}
/*
    parse update command
*/
DbOperator* parse_update(char* query_command, ClientContext* context, message* send_message) {
    int token_status = 0;
    char* token;
    char* endptr;
    Table* table = NULL;
    Column *col = NULL;
    GeneralizedColumn *pv = NULL;

    if (context == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
    parse_remove_parens(query_command, send_message);
    if (send_message->status == UNKNOWN_COMMAND || send_message->status == INCORRECT_FORMAT) return NULL;
   
    char** command_index = &query_command;

    token = next_token(command_index, COMMA, &token_status);

    col = lookup_column(token, &table);
    token = next_token(command_index, COMMA, &token_status);
    pv = lookup_generalized_column(token, context);
    
    if (col == NULL || pv == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
    token = next_token(command_index, COMMA, &token_status);

    DbOperator* dbo = malloc(sizeof(DbOperator));
    dbo->type = UPDATE;
    dbo->operator_fields.update_operator.table = table;
    dbo->operator_fields.update_operator.col_var = col;
    dbo->operator_fields.update_operator.pos_vec = pv;
    dbo->operator_fields.update_operator.update_value = strtol(token, &endptr, 10);
    send_message->status = OK_DONE;
    return dbo;
}
/*
    parse delete command
*/
DbOperator* parse_delete(char* query_command, ClientContext* context, message* send_message) {
    int token_status = 0;
    char* token;
    GeneralizedColumn *pv = NULL;

    if (context == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
    parse_remove_parens(query_command, send_message);
    if (send_message->status == UNKNOWN_COMMAND || send_message->status == INCORRECT_FORMAT) return NULL;
   
    char** command_index = &query_command;

   // parse table input
    char* table_name = next_token(command_index, COMMA, &token_status);
    if (token_status != 0) {
        send_message->status = INCORRECT_FORMAT;
        return NULL;
    }
    // lookup the table and make sure it exists. 
    Table* table = lookup_table(table_name);
    if (table == NULL) {
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }
    token = next_token(command_index, COMMA, &token_status);
    pv = lookup_generalized_column(token, context);
    if (pv == NULL){
        send_message->status = OBJECT_NOT_FOUND;
        return NULL;
    }

    DbOperator* dbo = malloc(sizeof(DbOperator));
    dbo->type = DELETE;
    dbo->operator_fields.delete_operator.table = table;
    dbo->operator_fields.delete_operator.pos_vec = pv;
    send_message->status = OK_DONE;
    return dbo;
}
/**
 * parse_command takes as input the send_message from the client and then
 * parses it into the appropriate query. Stores into send_message the
 * status to send back.
 * Returns a db_operator.
 **/
DbOperator* parse_command(char* query_command, message* send_message, int client_socket, ClientContext* context) {
    DbOperator *dbo = NULL; // = malloc(sizeof(DbOperator)); // calloc?
    static bool is_batching = false;

    if (strncmp(query_command, "--", 2) == 0) {
        send_message->status = OK_DONE;
        // The -- signifies a comment line, no operator needed.  
        return NULL;
    }

    char *equals_pointer = strchr(query_command, '=');
    char *handle = query_command;
    if (equals_pointer != NULL) {
        // handle exists, store here. 
        *equals_pointer = '\0';
        cs165_log(stdout, "FILE HANDLE: %s\n", handle);
        query_command = ++equals_pointer;
    } else {
        handle = NULL;
    }
#ifdef DEBUG
    cs165_log(stdout, "QUERY: %s\n", query_command);
#endif
    send_message->status = OK_WAIT_FOR_RESPONSE;
    query_command = trim_whitespace(query_command);
    // check what command is given. 
    if (strncmp(query_command, "create", 6) == 0) {
        query_command += 6;
        dbo = parse_create(query_command, send_message);
    } else if (strncmp(query_command, "load", 4) == 0) {
        query_command += 4;
        dbo = parse_load(query_command, send_message);
    } else if (strncmp(query_command, "relational_insert", 17) == 0) {
        query_command += 17;
        dbo = parse_insert(query_command, send_message);
    } else if (strncmp(query_command, "select", 6) == 0){
        query_command += 6;
        dbo = parse_select(query_command, handle, context, send_message);
    } else if (strncmp(query_command, "fetch", 5) == 0){
        query_command += 5;
        dbo = parse_fetch(query_command, handle, context, send_message);
    } else if (strncmp(query_command, "avg", 3) == 0){
        query_command += 3;
        dbo = parse_scalar_op(query_command, handle, context, send_message);
        if (dbo) dbo->operator_fields.scalar_operator.op_type = AVG;
    } else if (strncmp(query_command, "sum", 3) == 0){
        query_command += 3;
        dbo = parse_scalar_op(query_command, handle, context, send_message);
        if (dbo) dbo->operator_fields.scalar_operator.op_type = SUM;
    } else if (strncmp(query_command, "min", 3) == 0 ){
        query_command += 3;
        dbo = parse_scalar_op(query_command, handle, context, send_message);
        if (dbo) dbo->operator_fields.scalar_operator.op_type = MIN;
    } else if (strncmp(query_command, "max", 3) == 0 ){
        query_command += 3;
        dbo = parse_scalar_op(query_command, handle, context, send_message);
        if (dbo) dbo->operator_fields.scalar_operator.op_type = MAX;
    } else if (strncmp(query_command, "add", 3) == 0 ){
        query_command += 3;
        dbo = parse_arithmetic_op(query_command, handle, context, send_message);
        if (dbo) dbo->operator_fields.arithmetic_operator.op_type = ADD;
    } else if (strncmp(query_command, "sub", 3) == 0 ){
        query_command += 3;
        dbo = parse_arithmetic_op(query_command, handle, context, send_message);
        if (dbo) dbo->operator_fields.arithmetic_operator.op_type = SUB;
    } else if (strncmp(query_command, "shutdown", 8) == 0) {
        dbo = malloc(sizeof(DbOperator));
        dbo->type = CLOSE;
        send_message->status = OK_DONE;
    } else if (strncmp(query_command, "print", 5) == 0) {
        query_command += 5;
        dbo = parse_print(query_command, context, send_message);
    } else if (strstr(query_command, "batch_queries") != NULL) {
        is_batching = true;
        //qarray_initialize();
        tarray_initialize();
    } else if (strstr(query_command, "batch_execute") != NULL) {
        is_batching = false;
        dbo = malloc(sizeof(DbOperator));
        dbo->type = BATCH_EXECUTE;
    } else if (strncmp(query_command, "join", 4) == 0 ){
        query_command += 4;
        dbo = parse_join(query_command, handle, context, send_message);
    } else if (strncmp(query_command, "relational_update", 17) == 0) {
        query_command += 17;
        dbo = parse_update(query_command, context, send_message);
    } else if (strncmp(query_command, "relational_delete", 17) == 0) {
        query_command += 17;
        dbo = parse_delete(query_command, context, send_message);
    }

    if (dbo == NULL) {
        return dbo;
    }

    dbo->client_fd = client_socket;
    dbo->context = context;
    dbo->isbatched = false;
    if (is_batching){
        dbo->isbatched = true;

        PositionVector* res_pos = malloc(sizeof(PositionVector));
        add_position_vector_catalog_manager(dbo->operator_fields.select_operator.res_name, res_pos, dbo->context);
        add_to_task_array(dbo);     
    }
    return dbo;
}


