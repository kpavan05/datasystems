#ifndef __IOUTILS_H__
#define __IOUTILS_H__

#include <stdarg.h>
#include <stdio.h>
#include "cs165_api.h"

void delete_db_catalog();
FILE* open_catalog(char* mode);
void delete_db_folder(char* name);
int open_db_folder(char *name, char *parentname, bool create);
FILE* open_db_file(char *dbname, char *tblname, char* filename, char* mode);
void open_db_helper(char *buf, ClientContext* context, Status *ret_status);

void read_column_data(Column *column, Table *table, Status *ret_status);

void commit_column(Table* table, Column *column);
void commit_table(FILE* fpcatalog, Table *table);
void read_index(Table* table, Column* column);
void commit_index(Table* table, Column* column);
#endif