#include "client_context.h"
#include "utils.h"
/* This is an example of a function you will need to
 * implement in your catalogue. It takes in a string (char *)
 * and outputs a pointer to a table object. Similar methods
 * will be needed for columns and databases. How you choose
 * to implement the method is up to you.
 */
Table* lookup_table(char* name){
    int token_status;
    size_t ntables;
    char *tokenizer_copy, *to_free;
    char *token;

    tokenizer_copy = to_free= malloc((strlen(name)+1) * sizeof(char));
    strcpy(tokenizer_copy, name);
    tokenizer_copy = trim_quotes(tokenizer_copy);

    token = next_token(&tokenizer_copy, DOT, &token_status);
    if (token_status != 0) return NULL;
    if (token_status != 0 || strcmp(current_db->name, token) != 0){
        free(to_free);
        return NULL;
    }

    ntables = current_db->tables_size;
    token = next_token(&tokenizer_copy, DOT, &token_status);
    if (token_status != 0) {
        free(to_free);
        return NULL;
    }

    for (size_t i = 0;  i < ntables; i++){
        if (strcmp(current_db->tables[i].name, token) == 0){
            free(to_free);
            return &current_db->tables[i];
        }
    }
    free(to_free);
    return NULL;
}

Column* lookup_column(char* name, Table** tbl){
    int token_status;
    size_t ncolumns, ntables;
    char *tokenizer_copy, *to_free;
    char *token;
    Table *table = NULL;
    tokenizer_copy = to_free= malloc((strlen(name)+1) * sizeof(char));
    strcpy(tokenizer_copy, name);
    tokenizer_copy = trim_quotes(tokenizer_copy);
    
    token = next_token(&tokenizer_copy, DOT, &token_status);
    if (token_status != 0) {
        free(to_free);
        return NULL;
    }
    if (token_status != 0 || strcmp(current_db->name, token) != 0){
        free(to_free);
        return NULL;
    }

    ntables = current_db->tables_size;
    token = next_token(&tokenizer_copy, DOT, &token_status);
    if (token_status != 0) {
        free(to_free);
        return NULL;
    }

    for (size_t i = 0;  i < ntables; i++){
        if (strcmp(current_db->tables[i].name, token) == 0){
            table = &current_db->tables[i];
            break;
        }
    }
    if (table == NULL || table->col_count <= 0){
        free(to_free);
        return NULL;
    } 
    token = next_token(&tokenizer_copy, DOT, &token_status);
    if (token_status != 0) {
        free(to_free);
        return NULL;
    }
    *tbl = table;
    ncolumns = table->col_count;
    for(size_t i = 0; i < ncolumns; i++){
        if (strcmp(table->columns[i].name, token) == 0){
            free(to_free);
            return &table->columns[i];
        }
    }
    free(to_free);
    return NULL;
}

GeneralizedColumn* lookup_generalized_column(char* name, ClientContext *context){
    if (context == NULL || name[0] == '\0') return false;
    
    for(int i = 0; i < context->chandles_in_use; i++){
        if (strcmp(name, context->chandle_table[i].name) == 0){
            return &(context->chandle_table[i].generalized_column);
        }
    }
    return NULL;
}
