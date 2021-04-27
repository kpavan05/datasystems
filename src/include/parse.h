#ifndef PARSE_H__
#define PARSE_H__
#include "cs165_api.h"
#include "message.h"
#include "client_context.h"

long int parse_select_range(char** tokenizer_copy);
void parse_remove_parens(char* query_command, message* send_message);

DbOperator* parse_create(char* create_arguments, message *send_message);
message_status parse_create_db(char* create_arguments, DbOperator* dbo);
message_status parse_create_tbl(char* create_arguments, DbOperator* dbo);
message_status parse_create_col(char* create_arguments, DbOperator* dbo);
message_status parse_create_idx(char* create_arguments, DbOperator* dbo);

DbOperator* parse_load(char* query_command, message* send_message);
DbOperator* parse_insert(char* query_command, message* send_message) ;
DbOperator* parse_print(char* query_command, ClientContext* ctx, message* send_message);
DbOperator* parse_update(char* query_command, ClientContext* context, message* send_message);
DbOperator* parse_delete(char* query_command, ClientContext* context, message* send_message);
DbOperator* parse_select(char* query_command, char* handle, ClientContext* context, message* send_message);
DbOperator* parse_fetch(char* query_command, char* handle, ClientContext* context, message* send_message);
DbOperator* parse_scalar_op(char* query_command, char* handle, ClientContext* ctx, message* send_message);
DbOperator* parse_join(char* query_command, char* handle, ClientContext* context, message* send_message);
DbOperator* parse_command(char* query_command, message* send_message, int client, ClientContext* context);

#endif
