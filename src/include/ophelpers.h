#ifndef OP_HELPERS_H
#define OP_HELPERS_H

#include "cs165_api.h"

void db_fetch_select(PositionVector* source, PositionVector* dest);
void db_fetch_column(Column* column, PositionVector* posvec, Result* result);
void db_fetch_result(Result* source, PositionVector* posvec, Result* result);

void db_select_ge_op(long int icompare, size_t num_rows, GeneralizedColumn* curcolumn, unsigned int* ntuples, PositionVector* res_vec);
void db_select_greater_op(long int icompare, size_t num_rows, GeneralizedColumn* curcolumn, unsigned int* ntuples, PositionVector* res_vec);
void db_select_le_op(long int icompare, size_t num_rows, GeneralizedColumn* curcolumn, unsigned int* ntuples, PositionVector* res_vec);
void db_select_less_op(long int icompare, size_t num_rows, GeneralizedColumn* curcolumn, unsigned int* ntuples, PositionVector* res_vec);
void db_select_ge_less_op(long int icompare1, long int icompare2, size_t num_rows, GeneralizedColumn* curcolumn, unsigned int* ntuples, PositionVector* res_vec);


void db_calc_avg_i(size_t num_tuples,int* data, void* value);
void db_calc_avg_l(size_t num_tuples,long* data, void* value);
void db_calc_min_i(size_t num_tuples,int* data, void* value);
void db_calc_min_l(size_t num_tuples,long* data, void* value);
void db_calc_max_i(size_t num_tuples,int* data, void* value);
void db_calc_max_l(size_t num_tuples,long* data, void* value);
void db_calc_sum_i(size_t num_tuples,int* data, void* value);
void db_calc_sum_l(size_t num_tuples,long* data, void* value);

void db_calc_add_i(size_t nrows, int* data1, int* data2, Result* result);
void db_calc_sub_i(size_t nrows, int* data1, int* data2, Result* result);
void db_calc_add_l(size_t nrows, long* data1, long* data2, Result* result);
void db_calc_sub_l(size_t nrows, long* data1, long* data2, Result* result);
void db_calc_add_il(size_t nrows, long* data1, int* data2, Result* result);


extern int db_select_shared_ge_less_op(DbOperator* dbo, unsigned int start, unsigned int* ntuples, unsigned int* positions);
extern int db_select_shared_less_op(DbOperator* dbo, unsigned int start, unsigned int* ntuples, unsigned int* positions);
extern int db_select_shared_ge_op(DbOperator* dbo, unsigned int start, unsigned int* ntuples, unsigned int* positions);
extern int db_select_shared_greater_op(DbOperator* dbo, unsigned int start, unsigned int* ntuples, unsigned int* positions);
extern int db_select_shared_le_op(DbOperator* dbo,unsigned int start, unsigned int* ntuples, unsigned int* positions);

void db_select_index_ge_op(long int icompare, ColumnIndex* colindex, size_t nrows, unsigned int* ntuples, unsigned int* positions);
void db_select_index_le_op(long int icompare, ColumnIndex* colindex, size_t nrows, unsigned int* ntuples, unsigned int* positions);
void db_select_index_less_op(long int icompare, ColumnIndex* colindex, size_t nrows, unsigned int* ntuples, unsigned int* positions);
void db_select_index_greater_op(long int icompare, ColumnIndex* colindex,size_t nrows, unsigned int* ntuples,unsigned int* positions);
void db_select_index_ge_less_op(long int icompare1, long int icompare2, ColumnIndex* colindex, size_t nrows, unsigned int* ntuples, unsigned int* positions);
#endif