
#define _BSD_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include "ophelpers.h"


/* select greater than equal and less than operator*/
void db_select_ge_less_op_i(long int icompare1, long int icompare2, size_t num_rows, int* data,unsigned int* ntuples, unsigned int* positions){  
    unsigned bval = 0;
    unsigned int count = 0;
    for(size_t pos = 0; pos < num_rows; pos++){
        bval = data[pos] >= icompare1 && data[pos] < icompare2;
        positions[count] |= (bval*pos);
        count += bval;
    }
    *ntuples = count;
}

void db_select_ge_less_op_l(long int icompare1, long int icompare2, size_t num_rows, long* data,unsigned int* ntuples, unsigned int* positions){  
    unsigned bval = 0;
    unsigned int count = 0;
    for(size_t pos = 0; pos < num_rows; pos++){
        bval = data[pos] >= icompare1 && data[pos] < icompare2;
        positions[count] |= (bval*pos);
        count += bval;
    }
    *ntuples = count;
}
void db_select_ge_less_op(long int icompare1, long int icompare2, size_t num_rows, GeneralizedColumn* curcolumn,unsigned int* ntuples, PositionVector* res_vec){  
    unsigned int* positions = NULL;
    positions = (unsigned int*)calloc(num_rows, sizeof(unsigned int));
    memset(positions, 0, num_rows*sizeof(unsigned int));
    *ntuples = 0;

    if (curcolumn->column_type == COLUMN){
        ColumnIndex* colindex = curcolumn->column_pointer.column->index;
        if (colindex) {
            db_select_index_ge_less_op(icompare1, icompare2, colindex, num_rows, ntuples, positions);
        }
        else {
            int* idata = (int*)(curcolumn->column_pointer.column->data);
            db_select_ge_less_op_i(icompare1,icompare2, num_rows, idata, ntuples, positions);
        }
    }
    else{
        if (curcolumn->column_pointer.result->data_type == LONG){
            long* ldata = (long*)(curcolumn->column_pointer.result->payload);        
            db_select_ge_less_op_l(icompare1, icompare2, num_rows, ldata, ntuples, positions);
        }
        else {
            int* idata = (int*)(curcolumn->column_pointer.result->payload);
            db_select_ge_less_op_i(icompare1, icompare2, num_rows, idata, ntuples, positions);
        }
    }
    if (*ntuples < num_rows){
        res_vec->positions = realloc(positions, *ntuples*sizeof(unsigned int));
        if (!res_vec->positions) {
            free(res_vec->positions);
        }
    } else {
        res_vec->positions = positions;
    }
}

/* select for greater than equal operation */
void db_select_ge_op_i(long int icompare, size_t num_rows, int* data, unsigned int* ntuples, unsigned int* positions){  
    unsigned bval = 0;
    unsigned int count = 0;
    for(size_t pos = 0; pos < num_rows; pos++){
        bval = data[pos] >= icompare;
        positions[count] |= (bval*pos);
        count += bval;
    }
    *ntuples = count;
}

void db_select_ge_op_l(long int icompare, size_t num_rows, long* data, unsigned int* ntuples,unsigned int* positions){  
    unsigned bval = 0;
    unsigned int count = 0;
    for(size_t pos = 0; pos < num_rows; pos++){
        bval = data[pos] >= icompare;
        positions[count] |= (bval*pos);
        count += bval;
    }
    *ntuples = count;
}
void db_select_ge_op(long int icompare, size_t num_rows, GeneralizedColumn* curcolumn, unsigned int* ntuples, PositionVector* res_vec){  
    unsigned int* positions = NULL;
    positions = (unsigned int*)calloc(num_rows, sizeof(unsigned int));
    memset(positions, 0, num_rows*sizeof(unsigned int));
    *ntuples = 0;
    if (curcolumn->column_type == COLUMN){
        ColumnIndex* colindex = curcolumn->column_pointer.column->index;
        if (colindex) {
           db_select_index_ge_op(icompare, colindex, num_rows, ntuples, positions);
        } else {
            int* idata = (int*)(curcolumn->column_pointer.column->data);
            db_select_ge_op_i(icompare, num_rows, idata, ntuples, positions);
        }
    }
    else{
        if (curcolumn->column_pointer.result->data_type == LONG){
            long* ldata = (long*)(curcolumn->column_pointer.result->payload);        
            db_select_ge_op_l(icompare, num_rows, ldata, ntuples, positions);
        }
        else {
            int* idata = (int*)(curcolumn->column_pointer.result->payload);
            db_select_ge_op_i(icompare, num_rows, idata, ntuples, positions);
        }
    }
    
    if (*ntuples < num_rows){
        res_vec->positions = realloc(positions,*ntuples*sizeof(unsigned int));
        if (!res_vec->positions) {
            free(res_vec->positions);
        }
    } else {
        res_vec->positions = positions;
    }
}

/* select for greater operation */
void db_select_greater_op_i(long int icompare, size_t num_rows, int* data,unsigned int* ntuples, unsigned int* positions){  
    unsigned bval = 0;
    unsigned int count = 0;
    for(size_t pos = 0; pos < num_rows; pos++){
        bval = data[pos] > icompare;
        positions[count] |= (bval*pos);
        count += bval;
    }
    *ntuples = count;
}
void db_select_greater_op_l(long int icompare, size_t num_rows, long* data,unsigned int* ntuples, unsigned int* positions){  
    unsigned bval = 0;
    unsigned int count = 0;
    for(size_t pos = 0; pos < num_rows; pos++){
        bval = data[pos] > icompare;
        positions[count] |= (bval*pos);
        count += bval;
    }
    *ntuples = count;
}
void db_select_greater_op(long int icompare, size_t num_rows, GeneralizedColumn* curcolumn,unsigned int* ntuples, PositionVector* res_vec){  
    unsigned int* positions = NULL;
    positions = (unsigned int*)calloc(num_rows, sizeof(unsigned int));
    memset(positions, 0, num_rows*sizeof(unsigned int));
    *ntuples = 0;
    if (curcolumn->column_type == COLUMN){
        ColumnIndex* colindex = curcolumn->column_pointer.column->index;
        if (colindex) {
            db_select_index_greater_op(icompare, colindex, num_rows, ntuples, positions);
        } else {
            int* idata = (int*)(curcolumn->column_pointer.column->data);
            db_select_greater_op_i(icompare, num_rows, idata, ntuples, positions);
        }
    }
    else{
        if (curcolumn->column_pointer.result->data_type == LONG){
            long* ldata = (long*)(curcolumn->column_pointer.result->payload);        
            db_select_greater_op_l(icompare, num_rows, ldata, ntuples, positions);
        }
        else {
            int* idata = (int*)(curcolumn->column_pointer.result->payload);
            db_select_greater_op_i(icompare, num_rows, idata, ntuples, positions);
        }
    }
    if (*ntuples < num_rows){
        res_vec->positions = realloc(positions, *ntuples*sizeof(unsigned int));
        if (!res_vec->positions) {
            free(res_vec->positions);
        }
    } else {
        res_vec->positions = positions;
    }
}

/* select for less than equal operation*/
void db_select_le_op_i(long int icompare, size_t num_rows, int* data,unsigned int* ntuples, unsigned int* positions){  
    unsigned bval = 0;
    unsigned int count = 0;
    for(size_t pos = 0; pos < num_rows; pos++){
        bval = data[pos] <= icompare;
        positions[count] |= (bval*pos);
        count += bval;
    }   
    *ntuples = count;
}
void db_select_le_op_l(long int icompare, size_t num_rows, long* data,unsigned int* ntuples, unsigned int* positions){  
    unsigned bval = 0;
    unsigned int count = 0;   
    for(size_t pos = 0; pos < num_rows; pos++){
        bval = data[pos] <= icompare;
        positions[count] |= (bval*pos);
        count += bval;
    }
    *ntuples = count;
}
void db_select_le_op(long int icompare, size_t num_rows, GeneralizedColumn* curcolumn,unsigned int* ntuples, PositionVector* res_vec){
    unsigned int* positions = NULL;
    positions = (unsigned int*)calloc(num_rows, sizeof(unsigned int));
    memset(positions, 0, num_rows*sizeof(unsigned int));
    *ntuples = 0;
    if (curcolumn->column_type == COLUMN){
        ColumnIndex* colindex = curcolumn->column_pointer.column->index;
        if (colindex) {
            db_select_index_le_op(icompare, colindex, num_rows, ntuples, positions);
        } else {
            int* idata = (int*)(curcolumn->column_pointer.column->data);
            db_select_le_op_i(icompare, num_rows, idata, ntuples, positions);
        }
    }
    else{
        if (curcolumn->column_pointer.result->data_type == LONG){
            long* ldata = (long*)(curcolumn->column_pointer.result->payload);        
            db_select_le_op_l(icompare, num_rows, ldata, ntuples, positions);
        }
        else {
            int* idata = (int*)(curcolumn->column_pointer.result->payload);
            db_select_le_op_i(icompare, num_rows, idata, ntuples, positions);
        }
    }
    if (*ntuples < num_rows){
        res_vec->positions = realloc(positions, *ntuples*sizeof(unsigned int));
        if (!res_vec->positions) {
            free(res_vec->positions);
        }
    } else {
        res_vec->positions = positions;
    } 
}
/* select for less than operation*/
void db_select_less_op_i(long int icompare, size_t num_rows, int* data,unsigned int* ntuples, unsigned int* positions){ 
    unsigned bval = 0;
    unsigned int count = 0; 

    for(size_t pos = 0; pos < num_rows; pos++){
        bval = data[pos] < icompare;
        positions[count] |= (bval*pos);
        count += bval;
    }
    *ntuples = count;
}

void db_select_less_op_l(long int icompare, size_t num_rows, long* data,unsigned int* ntuples, unsigned int* positions){ 
    unsigned bval = 0;
    unsigned int count = 0;

    for(size_t pos = 0; pos < num_rows; pos++){
        bval = data[pos] < icompare;
        positions[count] |= (bval*pos);
        count += bval;
    }    
    *ntuples = count;
}

void db_select_less_op(long int icompare, size_t num_rows, GeneralizedColumn* curcolumn,unsigned  int* ntuples, PositionVector* res_vec){
    unsigned int* positions = NULL;
    positions = (unsigned int*)calloc(num_rows, sizeof(unsigned int));
    memset(positions, 0, num_rows*sizeof(unsigned int));
    *ntuples = 0;
    if (curcolumn->column_type == COLUMN){
        ColumnIndex* colindex = curcolumn->column_pointer.column->index;
        if (colindex) {
            db_select_index_less_op(icompare, colindex, num_rows, ntuples, positions);
        }
        else {
            int* idata = (int*)(curcolumn->column_pointer.column->data);
            db_select_less_op_i(icompare, num_rows, idata, ntuples, positions);
        }
    }
    else{
        if (curcolumn->column_pointer.result->data_type == LONG){
            long* ldata = (long*)(curcolumn->column_pointer.result->payload);        
            db_select_less_op_l(icompare, num_rows, ldata, ntuples, positions);
        }
        else {
            int* idata = (int*)(curcolumn->column_pointer.result->payload);
            db_select_less_op_i(icompare, num_rows, idata, ntuples, positions);
        }
    }
    if (*ntuples < num_rows){
        res_vec->positions = realloc(positions, *ntuples*sizeof(unsigned int));
        if (!res_vec->positions) {
            free(res_vec->positions);
        }
    } else {
        res_vec->positions = positions;
    }
}




/**********************************************************************
    index based select
**********************************************************************/
void db_select_index_ge_less_op(long int icompare1, long int icompare2, ColumnIndex* colindex, size_t nrows, unsigned int* ntuples, unsigned int* positions){

    *ntuples = 0;
    switch(colindex->type){
        case CLUSTER_BTREE:
            {
                int index1, index2;
                Node* node1 = btree_search(colindex->idxdata.treeidx.root, icompare1, COMPARE_GREATER_EQUAL, &index1);
                Node* node2 = btree_search(colindex->idxdata.treeidx.root, icompare2, COMPARE_LESS, &index2);
                if (index1 == -1 || index2 == -1) return;

                unsigned int endpos = node2->values[index2]->pos;
                unsigned int startpos = node1->values[index1]->pos;
                for(unsigned int i = startpos; i<= endpos; i++){
                    positions[(*ntuples)++] = i;
                }  
            }
            break;
        case UNCLUSTER_BTREE:
            {
                int index1, index2;
                Node* node1 = btree_search(colindex->idxdata.treeidx.root, icompare1, COMPARE_GREATER_EQUAL, &index1);
                Node* node2 = btree_search(colindex->idxdata.treeidx.root, icompare2, COMPARE_LESS, &index2);
                if (index1 == -1 || index2 == -1) return;

                int count = index1;
                int ulimit = node1 != node2 ? node1->size : index2 + 1;
                Node* node = node1;
                while (true) {
                    while (count < ulimit) {                        
                        record* rec = node->values[count];
                        while (rec) {positions[(*ntuples)++] = rec->pos; rec= rec->next;}
                        count++;
                    }
                    if (node == node2) break;
                    node = node->right;
                    ulimit = node == node2 ? index2 + 1 : node->size;
                    count = 0;
                }
            }
            break;
        case CLUSTER_SORT:
            {
                int* data = colindex->parent->data;
                int index1 = binary_search_data(data, 0, nrows-1, icompare1, COMPARE_GREATER_EQUAL);
                int index2 = binary_search_data(data, 0, nrows-1, icompare2, COMPARE_LESS);
                if (index1 == -1 || index2 == -1) return;
                for(int i=index1; i <= index2; i++) positions[(*ntuples)++] = i;
            }
            break;
        case UNCLUSTER_SORT:
            {
                //int* data = colindex->parent->data;
                int* data = colindex->idxdata.sortidx.copy_data;
                int index1 = binary_search_data(data, 0, nrows-1, icompare1, COMPARE_GREATER_EQUAL);
                int index2 = binary_search_data(data, 0, nrows-1, icompare2, COMPARE_LESS);
                if (index1 == -1 || index2 == -1) return;
                for(int i = index1; i <= index2; i++) positions[(*ntuples)++] = colindex->idxdata.sortidx.pv->positions[i];
            }
            break;
        default: break;
    }
}

void db_select_index_less_op(long int icompare, ColumnIndex* colindex, size_t nrows, unsigned int* ntuples, unsigned int* positions){

    *ntuples = 0;
    switch(colindex->type){
        case CLUSTER_BTREE:
            {   
                int index = -1;
                Node* node = btree_search(colindex->idxdata.treeidx.root, icompare, COMPARE_LESS, &index);
                if (index == -1) return;

                unsigned int endpos = node->values[index]->pos;
                for(unsigned int i = 0; i<= endpos; i++){
                    positions[i] = i;
                }
                *ntuples = endpos+1;      
            }
            break;
        case UNCLUSTER_BTREE:
            {
                int index = -1;
                Node* node = btree_search(colindex->idxdata.treeidx.root, icompare, COMPARE_LESS, &index);
                if (index == -1) return;

                int count = index;
                while (node) {
                    while (count >= 0) {
                        record* rec = node->values[count];
                        while (rec) {positions[(*ntuples)++] = rec->pos; rec= rec->next;}
                        count--;
                    }
                    node = node->left;
                    count = node->size-1;
                }
            }
            break;
        case CLUSTER_SORT:
            {
                int* data = colindex->parent->data;
                int index = binary_search_data(data, 0, nrows-1, icompare, COMPARE_LESS);
                if (index == -1) return;
                for(int i = 0; i < index; i++) positions[(*ntuples)++] = i;
            }
            break;
        case UNCLUSTER_SORT:
            {
                //int* data = colindex->parent->data;
                int* data = colindex->idxdata.sortidx.copy_data;
                int index = binary_search_data(data, 0, nrows-1, icompare, COMPARE_LESS);
                if (index == -1) return;
                for(int i = 0; i <= index; i++) positions[(*ntuples)++] = colindex->idxdata.sortidx.pv->positions[i];
            }
            break;
        default: break;
    }
}

void db_select_index_le_op(long int icompare, ColumnIndex* colindex, size_t nrows, unsigned int* ntuples, unsigned int* positions){

    *ntuples = 0;
    switch(colindex->type){
        case CLUSTER_BTREE:
            {
                int index = -1;
                Node* node = btree_search(colindex->idxdata.treeidx.root, icompare, COMPARE_LESS_EQUAL, &index);
                if (index == -1) return;

                unsigned int endpos = node->values[index]->pos;
                for(unsigned int i = 0; i<= endpos; i++){
                    positions[i] = i;
                }
                *ntuples = endpos+1;                  
            }
            break;       
        case UNCLUSTER_BTREE:
            {   
                int index = -1;
                Node* node = btree_search(colindex->idxdata.treeidx.root, icompare, COMPARE_LESS_EQUAL, &index);
                if (index == -1) return;

                int count = index;
                while(node){
                    while(count >= 0){
                        record* rec = node->values[count];
                        while (rec) {positions[(*ntuples)++] = rec->pos; rec= rec->next;}
                        count--;
                    }
                    node = node->left;
                    count = node->size-1;
                }                 
            }
            break;
        case CLUSTER_SORT:
            {
                int* data = colindex->parent->data;
                int index = binary_search_data(data, 0, nrows-1, icompare, COMPARE_LESS_EQUAL);
                if (index == -1) return;
                for(int i = 0; i <= index ; i++) positions[(*ntuples)++] = i;
            }
            break;
        case UNCLUSTER_SORT:
            {
                //int* data = colindex->parent->data;
                int* data = colindex->idxdata.sortidx.copy_data;
                int index = binary_search_data(data, 0, nrows-1, icompare, COMPARE_LESS_EQUAL);
                if (index == -1) return;
                for(int i = 0; i <= index ; i++) positions[(*ntuples)++] = colindex->idxdata.sortidx.pv->positions[i];
            }
            break;
        default: break;
    }
}
void db_select_index_ge_op(long int icompare, ColumnIndex* colindex, size_t nrows, unsigned int* ntuples, unsigned int* positions){
    *ntuples = 0;
    switch(colindex->type){
        case CLUSTER_BTREE:
            {
                int index = -1;
                Node* node = btree_search(colindex->idxdata.treeidx.root, icompare, COMPARE_GREATER_EQUAL, &index);
                if (index == -1) return;

                unsigned int startpos = node->values[index]->pos;
                for(unsigned int i = startpos; i < nrows; i++){
                    positions[(*ntuples)++] = i;
                }             
            }
            break;        
        case UNCLUSTER_BTREE:
            {
                int index = -1;
                Node* node = btree_search(colindex->idxdata.treeidx.root, icompare, COMPARE_GREATER_EQUAL, &index);
                if (index == -1) return;

                int count = index;
                while(node){
                    while(count < node->size){
                        record* rec = node->values[count];
                        while (rec) {positions[(*ntuples)++] = rec->pos; rec= rec->next;}
                        count++;
                    }
                    node = node->right;
                    count = 0;
                }                  
            }
            break;
        case CLUSTER_SORT:
            {
                int* data = colindex->parent->data;
                int index = binary_search_data(data, 0, nrows-1, icompare, COMPARE_GREATER_EQUAL);
                if (index == -1) return;
                for(unsigned int i = index; i < nrows; i++) positions[(*ntuples)++] = i;
            }
            break;
        case UNCLUSTER_SORT:
            {
                //int* data = colindex->parent->data;
                int* data = colindex->idxdata.sortidx.copy_data;
                int index = binary_search_data(data, 0, nrows-1, icompare, COMPARE_GREATER);
                if (index == -1) return;
                for(unsigned int i = index; i < nrows; i++) positions[(*ntuples)++] = colindex->idxdata.sortidx.pv->positions[i];
            }
            break;
        default: break;
    }
    
}
void db_select_index_greater_op(long int icompare, ColumnIndex* colindex,size_t nrows,unsigned int* ntuples,unsigned int* positions){
    *ntuples = 0;
    switch(colindex->type){
        case CLUSTER_BTREE:
            {
                int index = -1;
                Node* node = btree_search(colindex->idxdata.treeidx.root, icompare, COMPARE_GREATER, &index);
                if (index == -1) return;

                unsigned int startpos = node->values[index]->pos;
                for(unsigned int i = startpos; i < nrows; i++){
                    positions[(*ntuples)++] = i;
                }        
            }
            break; 
        case UNCLUSTER_BTREE:
            {
                int index = -1;
                Node* node = btree_search(colindex->idxdata.treeidx.root, icompare, COMPARE_GREATER, &index);
                if (index == -1) return;

                int count = index;
                while(node){
                    while(count < node->size){
                        record* rec = node->values[count];
                        while (rec) {positions[(*ntuples)++] = rec->pos; rec= rec->next;}
                        count++;
                    }
                    node = node->right;
                    count = 0;
                }                  
            }
            break;
        case CLUSTER_SORT:
            {
                int* data = colindex->parent->data;
                int index = binary_search_data(data, 0, nrows-1, icompare, COMPARE_GREATER);
                if (index == -1) return;
                for(unsigned int i = index; i < nrows; i++) positions[(*ntuples)++] = i;
            }
            break;
        case UNCLUSTER_SORT:
            {
                //int* data = colindex->parent->data;
                int* data = colindex->idxdata.sortidx.copy_data;
                int index = binary_search_data(data, 0, nrows-1, icompare, COMPARE_GREATER);
                if (index == -1) return;
                for(unsigned int i = index; i < nrows; i++) positions[(*ntuples)++] = colindex->idxdata.sortidx.pv->positions[i];
            }
            break;
        default: break;
    }
}


// binary search to find the key
/*
int binary_search(Node* node, int start, int end, int key, ComparatorType optype){
    int mid = start + (end-start)/2;
    switch(optype){
        case LESS_THAN_OR_EQUAL:
            if (key <= node->keys[mid] && key > node->keys[mid-1]) return mid;        
        case LESS_THAN:
            if (key < node->keys[mid] && key > node->keys[mid-1]) return mid;
        case GREATER_THAN_OR_EQUAL:
            if (key >= node->keys[mid] && key < node->keys[mid+1]) return mid;
        case GREATER_THAN:
            if (key > node->keys[mid] && key < node->keys[mid+1]) return mid;
        default:
            return -1;
    }   
    if (start >= end) return -1;
    if (key < node->keys[mid])
        return binary_search(node, start, mid, key, optype);
    else
        return binary_search(node, mid+1, end, key, optype);

}
*/
