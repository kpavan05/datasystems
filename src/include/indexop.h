
#ifndef INDEXOP_H
#define INDEXOP_H

#include "cs165_api.h"

Status db_btree_clustered(Table* table, Column* column, bool brestore);
Status db_btree_unclustered(Column* column);
Status db_sort_unclustered(Column* column);
Status db_sort_clustered(Table* table, Column* column, bool brestore);

void sort_data(size_t nvalues, int* data, int* copy, PositionVector* pv);
void propogate_sort_order(Column* column, unsigned int* positions);
void db_sort_columns(Table* table, Column* column, PositionVector* pv);

void db_insert_to_index(Table* table, Column* column, int pos, int* values, unsigned int posvalue);
void db_update_index(Table* table, Column* column, PositionVector* pv, int value);
void db_delete_index(Table* table, Column* column, PositionVector* pv);

#endif