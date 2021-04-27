#ifndef CLIENT_CONTEXT_H
#define CLIENT_CONTEXT_H

#include "cs165_api.h"

Table* lookup_table(char *name);
Column* lookup_column(char *name, Table** table);
GeneralizedColumn* lookup_generalized_column(char* name, ClientContext *context);
#endif
