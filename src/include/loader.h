#ifndef __LOADER_H__
#define __LOADER_H__

#include <stdarg.h>
#include <stdio.h>
#include "cs165_api.h"

Db* fetch_db(char*  name);
Table* fetch_table(Db* db, char* name);
int read_header(char* line, Table** table);
void insert_data(char* line, Table* table);

#endif