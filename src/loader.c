#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "message.h"
#include "utils.h"
#include "loader.h"

/*
    reads the header of the file that is passed to load command
    identifies the table and columns to be created or opened.
*/
int read_header(char* line, Table** tbl){
	int token_status;
	size_t col_num = 0;
	char *token;
	char *db_name;
	char *tbl_name;
	char *col_name;
	char *tokenizer_copy;
    Db *db = NULL;
    Table *table = NULL;

    tokenizer_copy = malloc((strlen(line) + 1)*sizeof(char)); 
    strcpy(tokenizer_copy, line);
    tokenizer_copy = trim_quotes(tokenizer_copy);
    tokenizer_copy = trim_newline(tokenizer_copy);
    token = next_token(&tokenizer_copy, COMMA, &token_status);
    
    while(token != NULL && token[0] != '\0' && token_status == 0){
    	db_name = next_token(&token, DOT, &token_status);
        if ((db = fetch_db(db_name)) == NULL){
            return -1;
        }

        tbl_name = next_token(&token, DOT, &token_status);
        if ((table = fetch_table(db, tbl_name)) == NULL){
            return -1;
        }

      	col_name = next_token(&token, DOT, &token_status);
        col_num++;
        if (!has_column(table,col_name)){
            return -1;
        }
        token = next_token(&tokenizer_copy, COMMA, &token_status);
        
    }
    if (table->col_count > col_num){
        //log error does not have all columns listed.
        return -1;
    }
    *tbl = table;
    return 0;
}

Db* fetch_db(char*  name){
    if (current_db == NULL) {
        //log error no db exists
        return NULL;
    }
    if (strcmp(current_db->name, name) != 0) {
        //log error db does not match
        return NULL;
    }
    return current_db;
}

Table* fetch_table(Db* db, char* name){
    size_t ntables = db->tables_size;
    for(size_t i = 0; i < ntables; i++){
        if (strcmp(db->tables[i].name, name) == 0){
            return &db->tables[i];
        }
    }
    //log error tables does not exist
    return NULL;
}
/*
    push data from the file given to load command to appropriate table
    i.e. data tokenized by comma operator in the csv file is pushed to 
    respective column data structure.
*/
void insert_data(char* line, Table* table){
    char *token;
    char *endptr;
    long val;
    int col_num = 0;
    int token_status = 0;
    size_t capacity;
    char *tokenizer_copy;

    tokenizer_copy = malloc((strlen(line) + 1)*sizeof(char));
    strcpy(tokenizer_copy, line);

    tokenizer_copy = trim_newline(tokenizer_copy);
    token = next_token(&tokenizer_copy, COMMA, &token_status);   
    while(token != NULL && token[0] != '\0' && token_status == 0){ 
       val = strtol(token, &endptr, 10);
       capacity = table->columns[col_num].col_capacity;

       //realloc the memory for data
       if (table->table_length == capacity){
            table->columns[col_num].col_capacity = 2*capacity;
            table->columns[col_num].data = realloc(table->columns[col_num].data, 2*capacity*sizeof(int));
       }

       table->columns[col_num].isdirty = true;
       table->columns[col_num].data[table->table_length] = val;
       
       token = next_token(&tokenizer_copy, COMMA, &token_status); 
       col_num++;
    }
    free(tokenizer_copy);
}

