#ifndef _JOIN_H
#define _JOIN_H

#include "cs165_api.h"

void db_nested_join_i(int* payload1, int* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2);
void db_nested_join_l(long* payload1, long* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2);
void db_hash_join_i(int* payload1, int* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2);
void db_hash_join_l(long* payload1, long* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2);
void db_block_nested_join_i(int* payload1, int* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2);
void db_sort_join_i(int* payload1, int* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2);
void db_grace_join_i(int* payload1, int* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2);
#endif