#define _BSD_SOURCE
#include <sys/time.h>
#include <limits.h>
#include "indexop.h"
#include "btree.h"
#include "utils.h"
Status db_btree_clustered(Table* table, Column* column, bool brestore){
	Status ret_status;
	int *idata = column->data;
	unsigned int nitems = column->col_size;

    column->index->idxdata.treeidx.root = NULL;
	if (!brestore){
		PositionVector* pv = malloc(sizeof(PositionVector));
		pv->positions = NULL;
		pv->num_tuples = nitems;
		sort_data(nitems, idata, NULL, pv);
		db_sort_columns(table, column, pv);
		free(pv->positions);
		free(pv);
	}
	
	/*
	struct timeval st, et;
    gettimeofday(&st,NULL);
	for(unsigned int i = 0; i < nitems; i++){
		//btree_insert(&column->index->root, idata[i], i);
		btree_insert(&(column->index->idxdata.treeidx.root), idata[i], &i);
	}
	gettimeofday(&et,NULL);
    long elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    cs165_log(stdout, "--Time taken for batch run %ld usec\n", elapsed);
	*/
	unsigned int* positions = calloc(nitems, sizeof(unsigned int));
	for(unsigned int i = 0; i < nitems; i++) positions[i] = i;
	/*	
	struct timeval st, et;
    gettimeofday(&st,NULL);
    */
	btree_bulk_insert(&(column->index->idxdata.treeidx.root), idata, positions, nitems);
	/*
    gettimeofday(&et,NULL);
    long elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    cs165_log(stdout, "--Time taken for bulk insertion run %ld usec\n", elapsed);
    */
	ret_status.code = OK;
	return ret_status;
}

Status db_btree_unclustered(Column* column){
	Status ret_status;
	int *idata = column->data;
	size_t nitems = column->col_size;
    /*
	struct timeval st, et;
    gettimeofday(&st,NULL);
    */
	PositionVector* pv = malloc(sizeof(PositionVector));
	pv->positions = NULL;
	pv->num_tuples = nitems;

	column->index->idxdata.treeidx.root = NULL;
	int* copy_data = (int*)calloc(column->col_size, sizeof(int));
	//column->index->idxdata.treeidx.copy_data = (int*)calloc(column->col_size, sizeof(int));
	sort_data(nitems, idata, copy_data, pv);
	
	
	for(size_t i = 0; i < nitems; i++){
		btree_insert(&(column->index->idxdata.treeidx.root), copy_data[i], &pv->positions[i]);
	}
		
	/*
	struct timeval st, et;
    gettimeofday(&st,NULL);

	btree_bulk_insert(&(column->index->idxdata.treeidx.root), copy_data, pv->positions, nitems);
	
	gettimeofday(&et,NULL);
    long elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    cs165_log(stdout, "--Time taken for bulk insertion run %ld usec\n", elapsed);
    */
	free(pv->positions);
	free(pv);
    /*
	gettimeofday(&et,NULL);
    long elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    cs165_log(stdout, "--Time taken for btree normal insert run %ld usec\n", elapsed);
    */
	ret_status.code = OK;
	return ret_status;
}

Status db_sort_clustered(Table* table, Column* column, bool brestore){
	Status ret_status;
	int* data = (int*)column->data;

	column->index->idxdata.sortidx.copy_data = NULL;
	column->index->idxdata.sortidx.pv = malloc(sizeof(PositionVector));
	if (!brestore) {
		sort_data(column->col_size, data, NULL, column->index->idxdata.sortidx.pv);
		db_sort_columns(table, column, column->index->idxdata.sortidx.pv);
	} else {
		column->index->idxdata.sortidx.pv->num_tuples = column->col_size;
		column->index->idxdata.sortidx.pv->positions = calloc(column->col_size, sizeof(unsigned int));
		for(unsigned int i=0; i< column->col_size; i++){
			column->index->idxdata.sortidx.pv->positions[i]= i;
		}
	}
	
	ret_status.code = OK;
	return ret_status;
}

Status db_sort_unclustered(Column* column){
	Status ret_status;
	int* data = (int*)column->data;

	column->index->idxdata.sortidx.pv = malloc(sizeof(PositionVector));
	column->index->idxdata.sortidx.pv->num_tuples = column->col_size;
	column->index->idxdata.sortidx.copy_data = (int*)calloc(column->col_size, sizeof(int));
	sort_data(column->col_size, data, column->index->idxdata.sortidx.copy_data, column->index->idxdata.sortidx.pv);
	ret_status.code = OK;
	return ret_status;
}

void db_sort_columns(Table* table, Column* column, PositionVector* pv){
	int len = table->col_count;

	for(int i = 0; i < len; i++){
		if (strcmp(table->columns[i].name, column->name) == 0) continue;
		propogate_sort_order(&table->columns[i], pv->positions);
	}
}

void propogate_sort_order(Column* column, unsigned int* positions){
	size_t len = column->col_size;
	int* idata = column->data;
    int* idxdata = calloc(len, sizeof(int));

	for(size_t i= 0; i < len; i++){
		idxdata[i] = idata[positions[i]];
	}
	free(column->data);
	column->data = idxdata;
}

/***************************************************************************
	insert to index
****************************************************************************/
void db_insert_btree_uncluster(Column* column, int value, unsigned int pos){
	btree_insert(&(column->index->idxdata.treeidx.root), value, &pos);
	/*
	for(size_t j = nrows; j > index; j--){
		curcolumn->index->idxdata.treeidx.copy_data[j] = curcolumn->index->idxdata.sortidx.copy_data[j-1];
	}
	curcolumn->index->idxdata.treeidx.copy_data[index] = value;
	*/
}
void db_insert_sort_uncluster(Column* column,int value, unsigned int pos){
	unsigned int nrows = column->col_size;
	int index = binary_search_data(column->index->idxdata.sortidx.copy_data, 0, column->col_size-1, value, COMPARE_GREATER_EQUAL);
	unsigned int uindex = (unsigned) index;
	int* copy = column->index->idxdata.sortidx.copy_data;
	for(size_t i = 0; i < 10; i++){
        cs165_log(stdout, "%d:%d,%u\n", i, copy[i], column->index->idxdata.sortidx.pv->positions[i]);
    }
    	
	for(unsigned int j = nrows; j > uindex; j--){
		copy[j] = copy[j-1];
	}
	
	for(unsigned int j = nrows-1; j != UINT_MAX; j--){
		column->index->idxdata.sortidx.pv->positions[j+(j>=uindex)] = column->index->idxdata.sortidx.pv->positions[j] + (column->index->idxdata.sortidx.pv->positions[j] >= pos);
	}
	copy[index] = value;
	column->index->idxdata.sortidx.pv->positions[index] = pos;
	column->index->idxdata.sortidx.pv->num_tuples++;
	for(size_t i = 0; i < 10; i++){
        cs165_log(stdout, "%d:%d,%u\n", i, copy[i],column->index->idxdata.sortidx.pv->positions[i]);
    }
}
void db_insert_btree_cluster(Table* table, Column* column, int pos, int* values){
	unsigned int index = UINT_MAX;
	int* curdata = NULL;
	curdata = (int*)column->data;
	btree_insert(&(column->index->idxdata.treeidx.root), values[pos], &index);

	size_t ncolumns = table->col_count;
	for(size_t i = 0; i < ncolumns; i++){
		Column* curcolumn = &table->columns[i];
		size_t nrows = curcolumn->col_size;
		curdata = curcolumn->data;
		for(size_t j = nrows; j > index; j--){
			curdata[j] = curdata[j-1];
		}
		curdata[index] = values[i];

		if (curcolumn->index && curcolumn->index->type == UNCLUSTER_BTREE)
			db_insert_btree_uncluster(curcolumn, values[i], index);
        if (curcolumn->index && curcolumn->index->type == UNCLUSTER_SORT)
           	db_insert_sort_uncluster(curcolumn, values[i], index);

		curcolumn->col_size += 1;
	}
}
void db_insert_sort_cluster(Table* table, Column* column, int pos, int* values){
	int index = binary_search_data(column->data, 0, column->col_size-1, values[pos], COMPARE_GREATER_EQUAL);
	if (index == -1) index = 0;
	
	int nrows = (int)column->col_size;
	for (int j = nrows; j > index; j--){
		column->index->idxdata.sortidx.pv->positions[j]= column->index->idxdata.sortidx.pv->positions[j-1] + 1;
	}
	column->index->idxdata.sortidx.pv->positions[index] = index;
	column->index->idxdata.sortidx.pv->num_tuples++;

	int* curdata = NULL;
	size_t ncolumns = table->col_count;
	for(size_t i = 0; i < ncolumns; i++){
		Column* curcolumn = &table->columns[i];
		curdata = curcolumn->data;
		for(int j = nrows; j > index; j--){
			curdata[j] = curdata[j-1];
		}
		curdata[index] = values[i];

		if (curcolumn->index && curcolumn->index->type == UNCLUSTER_BTREE)
			db_insert_btree_uncluster(curcolumn, values[i], index);
        if (curcolumn->index && curcolumn->index->type == UNCLUSTER_SORT)
           	db_insert_sort_uncluster(curcolumn, values[i], index);

		curcolumn->col_size += 1;
	}
}

void db_insert_to_index(Table* table, Column* column, int pos, int* values, unsigned int posvalue){
    if (column == NULL || column->index == NULL) return;
    switch(column->index->type){
        case CLUSTER_BTREE: 
        	db_insert_btree_cluster(table, column, pos, values);
        	break;
        case CLUSTER_SORT:  
        	db_insert_sort_cluster(table, column, pos, values);
        	break;
        break;
        case UNCLUSTER_SORT: 
        	db_insert_btree_uncluster(column, values[pos], posvalue);  
        	break;
        case UNCLUSTER_BTREE: 
        	db_insert_btree_uncluster(column, values[pos], posvalue);
        	break;
        default: break;
    }
}

/***************************************************************************
	update index
****************************************************************************/
void db_modify_sort_uncluster(Column* column){	
	int* data = column->data;
	int* copy = column->index->idxdata.sortidx.copy_data;
	int index = -1;

	for(unsigned int i=0; i< column->col_size; i++){
		index = binary_search_data(copy, 0, column->col_size-1, data[i], COMPARE_GREATER_EQUAL);
		column->index->idxdata.sortidx.pv->positions[index] = i;
	}
}
void db_modify_btree_uncluster(Column* column){
	int* data = column->data;
	int index = -1;
	Node* curnode = NULL;
	Node* root = column->index->idxdata.treeidx.root;

	for(unsigned int i=0; i< column->col_size; i++){
		curnode = btree_search(root, data[i], COMPARE_GREATER_EQUAL, &index);
		curnode->values[index]->pos = i;
	}
}
void db_update_btree_uncluster(Column* column, PositionVector* pv, int value){
	int* data = column->data;
	Node* root = column->index->idxdata.treeidx.root;
	for(unsigned int i = 0; i < pv->num_tuples; i++){
    	btree_delete(root, data[pv->positions[i]], false);
    	btree_insert(&root, value, &pv->positions[i]);
    }
}
void db_update_sort_uncluster(Column* column, PositionVector* pv, int value){	
	int* data = column->data;
	int* copy = column->index->idxdata.sortidx.copy_data;
	int index = -1;
	int numrows = (int)column->col_size;
	for(unsigned int i = 0; i < pv->num_tuples; i++){
		index = binary_search_data(copy, 0, column->col_size-1, data[pv->positions[i]], COMPARE_GREATER_EQUAL);
		if (value > copy[index + 1]){
			for(int i = index; i < numrows; i++){
				if (value <= copy[i]){
					copy[i-1] = value;
					column->index->idxdata.sortidx.pv->positions[i-1] = index;
				}
				copy[i] = copy[i+1];
				column->index->idxdata.sortidx.pv->positions[i] = column->index->idxdata.sortidx.pv->positions[i+1];
			}
			
		} else {
			for(int i = index; i > 0; i--){
				if (value >= copy[i]){
					copy[i+1] = value;
					column->index->idxdata.sortidx.pv->positions[i+1] = index;
				}
				copy[i] = copy[i-1];
				column->index->idxdata.sortidx.pv->positions[i] = column->index->idxdata.sortidx.pv->positions[i+1];
			}
		}
	}
}
void db_update_btree_cluster(Table* table, Column* column, PositionVector* pv, int value){
	int* data = column->data;
	unsigned int index;
	unsigned int* searchpos = calloc(pv->num_tuples, sizeof(unsigned int));
	Node* root = column->index->idxdata.treeidx.root;
    for(unsigned int i = 0; i < pv->num_tuples; i++){
    	btree_delete(root, data[pv->positions[i]], true);
    	index = UINT_MAX;
    	btree_insert(&root, value, &index);
    	searchpos[i] = index;
    }
	
    int curvalue;
	size_t ncolumns = table->col_count;
	for(size_t i = 0; i < ncolumns; i++){
		Column* curcolumn = &table->columns[i];

        int* col_data = curcolumn->data;
        for(unsigned int i = 0; i < pv->num_tuples; i++){		
			if (pv->positions[i] > searchpos[i]) {
				curvalue = col_data[pv->positions[i]];
				for(unsigned int j = pv->positions[i]; j > searchpos[i]; j--){
					col_data[j] = col_data[j-1];
				}
				col_data[searchpos[i]] = curcolumn != column ? curvalue : value;
			} else {
				curvalue = col_data[pv->positions[i]];
				for(unsigned int j = pv->positions[i]; j < searchpos[i]; j++){
					col_data[j] = col_data[j+1];
				}
				col_data[searchpos[i]] = curcolumn != column ? curvalue : value;
			}
		}
		if (curcolumn->index && curcolumn->index->type == UNCLUSTER_BTREE)
			db_modify_btree_uncluster(curcolumn);
        if (curcolumn->index && curcolumn->index->type == UNCLUSTER_SORT)
           	db_modify_sort_uncluster(curcolumn);       
	}
	free(searchpos);
}
void db_update_sort_cluster(Table* table, Column* column, PositionVector* pv, int updatevalue){
	int* data = column->data;
	
	unsigned int* searchpos = calloc(pv->num_tuples, sizeof(unsigned int));
	for(unsigned int i = 0; i < pv->num_tuples; i++){
		searchpos[i] = (unsigned)(binary_search_data(data, 0, column->col_size-1, updatevalue, COMPARE_GREATER_EQUAL));
		if (pv->positions[i] > searchpos[i]){
			for(unsigned int j = pv->positions[i]; j > searchpos[i]; j--){
				data[j] = data[j-1];
			}
			data[searchpos[i]] = updatevalue;
		} else{
			for(unsigned int j = pv->positions[i]; j < searchpos[i]; j++){
				data[j] = data[j+1];
			}
			data[searchpos[i]] = updatevalue;
		}
	}
	int curvalue;
	size_t ncolumns = table->col_count;
	for(size_t i = 0; i < ncolumns; i++){
		Column* curcolumn = &table->columns[i];
		if (curcolumn == column) continue;

        int* col_data = curcolumn->data;
        for(unsigned int i = 0; i < pv->num_tuples; i++){		
			if (pv->positions[i] > searchpos[i]) {
				curvalue = col_data[pv->positions[i]];
				for(unsigned int j = pv->positions[i]; j > searchpos[i]; j--){
					col_data[j] = col_data[j-1];
				}
				col_data[searchpos[i]] = curvalue;
			} else {
				curvalue = col_data[pv->positions[i]];
				for(unsigned int j = pv->positions[i]; j < searchpos[i]; j++){
					col_data[j] = col_data[j+1];
				}
				col_data[searchpos[i]] = curvalue;
			}
		}
		if (curcolumn->index && curcolumn->index->type == UNCLUSTER_BTREE)
			db_modify_btree_uncluster(curcolumn);
        if (curcolumn->index && curcolumn->index->type == UNCLUSTER_SORT)
           	db_modify_sort_uncluster(curcolumn);       
	}
	free(searchpos);
}
void db_update_index(Table* table, Column* column, PositionVector* pv, int value){
    if (column == NULL || column->index == NULL) return;
    switch(column->index->type){
        case CLUSTER_BTREE: 
        	db_update_btree_cluster(table, column, pv, value);
        	break;
        case CLUSTER_SORT:  
        	db_update_sort_cluster(table, column, pv, value);
        	break;
        case UNCLUSTER_SORT: 
        	db_update_sort_uncluster(column, pv, value);  
        	break;
        case UNCLUSTER_BTREE: 
        	db_update_btree_uncluster(column, pv, value);
        	break;
        default: break;
    }
}
/***************************************************************************
	delete on index
****************************************************************************/
void db_delete_btree_uncluster(Column* column, PositionVector* pv){
	unsigned int nitems = pv->num_tuples;
	int* data = column->data;

	for(unsigned int i = 0; i < nitems; i++){
		btree_delete(column->index->idxdata.treeidx.root, data[pv->positions[i]], false);
	}	
}
void db_delete_sort_uncluster(Column* column, PositionVector* pv){	
	int curindex;
	int value;
	unsigned int numrows = column->col_size;	
	unsigned int nitems = pv->num_tuples;	
	int* data = column->data;

	/* collect the positions that need to be deleted in index position vector*/
	for(unsigned int i = 0; i < nitems; i++){
		value = data[pv->positions[i]];
	    curindex = binary_search_data(column->index->idxdata.sortidx.copy_data, 0, column->col_size-1, value, COMPARE_GREATER_EQUAL);
	    for(unsigned int j = curindex; j < numrows; j++){
	    	column->index->idxdata.sortidx.copy_data[j] = column->index->idxdata.sortidx.copy_data[j+1];
	    	column->index->idxdata.sortidx.pv->positions[j] = column->index->idxdata.sortidx.pv->positions[j+1];
	    	column->index->idxdata.sortidx.pv->positions[j] -= (column->index->idxdata.sortidx.pv->positions[j] > pv->positions[i]);
	    }
	    for(unsigned int j = 0; j < (unsigned)curindex; j++){
	    	column->index->idxdata.sortidx.pv->positions[j] -= (column->index->idxdata.sortidx.pv->positions[j] > pv->positions[i]);
	    }
	    numrows--;
	}
	column->index->idxdata.sortidx.pv->num_tuples -= nitems;
	column->index->idxdata.sortidx.copy_data = realloc(column->index->idxdata.sortidx.copy_data, numrows* sizeof(int));
}

void db_delete_btree_cluster(Table* table, Column* column, PositionVector* pv){
	unsigned int nitems = pv->num_tuples;
	unsigned int numrows = column->col_size;
	unsigned int count = 0;
	int* data = column->data;
	for(unsigned int i = 0; i < nitems; i++){
		btree_delete(column->index->idxdata.treeidx.root, data[pv->positions[i]], true);
	}
	int* curdata = NULL;
	size_t ncolumns = table->col_count;
	for(size_t i = 0; i < ncolumns; i++){
		Column* curcolumn = &table->columns[i];

		if (curcolumn->index && curcolumn->index->type == UNCLUSTER_BTREE)
			db_delete_btree_uncluster(curcolumn, pv);
        if (curcolumn->index && curcolumn->index->type == UNCLUSTER_SORT)
           	db_delete_sort_uncluster(curcolumn, pv);

        count = 0; 
        curdata = curcolumn->data;  
		numrows = curcolumn->col_size;
		for(unsigned int pos = 0; pos < numrows; pos++){
			count += (pv->positions[count] == pos);
			curdata[pos] = curdata[pos + count];
			numrows--;
	    }
		curcolumn->col_size = curcolumn->col_size - pv->num_tuples;
	}
}

void db_delete_sort_cluster(Table* table, Column* column, PositionVector* pv){
	unsigned int count = 0;
	unsigned int numrows = column->col_size;
	if (pv == NULL || pv->num_tuples == 0) return;
	
	int* curdata = column->data;
	for(unsigned int pos = 0; pos < numrows; pos++){
		count += (pv->positions[count] == pos);
		curdata[pos] = curdata[pos + count];
		column->index->idxdata.sortidx.pv->positions[pos] = pos - count;
		numrows--;
	}
	column->col_size -= pv->num_tuples;
	column->index->idxdata.sortidx.pv->num_tuples -= pv->num_tuples;

	size_t ncolumns = table->col_count;
	for(size_t i = 0; i < ncolumns; i++){
		Column* curcolumn = &table->columns[i];
		if (curcolumn == column) continue;

		if (curcolumn->index && curcolumn->index->type == UNCLUSTER_BTREE)
			db_delete_btree_uncluster(curcolumn, pv);
        if (curcolumn->index && curcolumn->index->type == UNCLUSTER_SORT)
           	db_delete_sort_uncluster(curcolumn, pv);

        count = 0; 
        curdata = curcolumn->data;  
		numrows = curcolumn->col_size;
		/*
		for(size_t i = 0; i < 15; i++){
        	cs165_log(stdout, "%d:%d\n", i, curdata[i]);
    	}
    	*/
		for(unsigned int pos = 0; pos < numrows; pos++){
			count += (pv->positions[count] == pos);
			curdata[pos] = curdata[pos + count];
	    }
		curcolumn->col_size = curcolumn->col_size - pv->num_tuples;
		/*
    	for(size_t i = 0; i < 15; i++){
        	cs165_log(stdout, "%d:%d\n", i, curdata[i]);
    	}
    	*/
	}
}
void db_delete_index(Table* table, Column* column, PositionVector* pv){
    if (column == NULL || column->index == NULL) return;
    switch(column->index->type){
        case CLUSTER_BTREE: 
        	db_delete_btree_cluster(table, column, pv);
        	break;
        case CLUSTER_SORT:  
        	db_delete_sort_cluster(table, column, pv);
        	break;
        case UNCLUSTER_SORT: 
        	db_delete_sort_uncluster(column, pv);  
        	break;
        case UNCLUSTER_BTREE: 
        	db_delete_btree_uncluster(column, pv);
        	break;
        default: break;
    }
}

/*
void db_delete_sort_cluster(Table* table, Column* column, PositionVector* pv){
	int bval;
	unsigned int pos, count;
	unsigned int numrows = column->col_size;

	int* curdata = column->data;
	int* newdata = calloc(numrows- pv->num_tuples, sizeof(int));
	pos = 0; count = 0;
	for(unsigned int id = 0; id < numrows; id++){
		bval = pv->positions[pos] != id;
		newdata[count] |= curdata[id]*bval;
		column->index->idxdata.sortidx.pv->positions[count] -= count;
		pos += pv->positions[pos] == id;
		count += bval;
	}
	free(column->data);
	column->data = newdata;
	column->col_size -= pv->num_tuples;
	column->index->idxdata.sortidx.pv->num_tuples -= pv->num_tuples;
	column->col_size = column->col_size - pv->num_tuples;

	size_t ncolumns = table->col_count;
	for(size_t i = 0; i < ncolumns; i++){
		Column* curcolumn = &table->columns[i];
		if (curcolumn == column) continue;

		if (curcolumn->index && curcolumn->index->type == UNCLUSTER_BTREE)
			db_delete_btree_uncluster(curcolumn, pv);
        if (curcolumn->index && curcolumn->index->type == UNCLUSTER_SORT)
           	db_delete_sort_uncluster(curcolumn, pv);

		numrows = curcolumn->col_size;
		curdata = curcolumn->data;
		newdata = calloc(numrows- pv->num_tuples, sizeof(int));
		pos = 0; count = 0; bval = 0;
		for(unsigned int id = 0; id < numrows; id++){
			bval = pv->positions[pos] != id;
			newdata[count] |= curdata[id]*bval;
			pos += pv->positions[pos] == id;
			count += bval;
		}
		free(curcolumn->data);
		curcolumn->data = newdata;
		curcolumn->col_size = curcolumn->col_size - pv->num_tuples;		
	}
}
*/