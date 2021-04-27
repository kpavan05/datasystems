#define _BSD_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include "indexop.h"

typedef struct sortunit{
	int value;
	unsigned int pos;
}sortunit;


size_t partition(size_t start, size_t end, int* data, PositionVector* pv){
	size_t pivot_pos;
	int temp = -1;
	size_t pos = 0;	
	int pivot = data[end];
	
	pivot_pos = start-1;
	for(size_t i = start; i <= end-1; i++){
		if (data[i] <= pivot){
			pivot_pos++;
			temp = data[pivot_pos];
			data[pivot_pos] = data[i];
			data[i] = temp;
			pos = pv->positions[i];
			pv->positions[i] = pv->positions[pivot_pos];
			pv->positions[pivot_pos] = pos;		
		}
	}
	pivot_pos++;
	temp = data[pivot_pos];
	data[pivot_pos] = data[end];
	data[end] = temp;
	pos = pv->positions[end];
	pv->positions[end] = pv->positions[pivot_pos];
	pv->positions[pivot_pos] = pos;
	return pivot_pos;

}
void qsort_data(int* data, size_t start, size_t end, PositionVector* pv){
	int partition_pos;
	if (start < end){
    	partition_pos = partition(start, end, data, pv);
		qsort_data(data, start, partition_pos - 1, pv);
		qsort_data(data, partition_pos+1, end, pv);
	}
}

void insertion_sort(int* data, size_t nvalues, PositionVector* pv){
	unsigned int pos = 0;
	unsigned int i = 0, j=0;
	int val ;

	for(i = 1; i < nvalues; i++){
		val = data[i];
		pos = i;
		for(j = i-1; j != UINT_MAX && val < data[j]; j--){
			data[j+1] = data[j];
			pv->positions[j+1] = j;
		}
		data[j+1] = val;
		pv->positions[j+1] = pos;
	}

}

void merge_sortunit(sortunit* pdata, sortunit* resdata, size_t start, size_t end){
	if (start >= end) {
		return;
	}
	size_t i, j, pos;
	size_t mid = start + (end-start)/2;
	i = start; j = mid+1;
	pos = start;
	merge_sortunit(pdata, resdata, start, mid);
	merge_sortunit(pdata, resdata, mid+1, end);

	while ( pos <= end){
		if (j > end){
			resdata[pos].value = pdata[i].value;
			resdata[pos++].pos = pdata[i++].pos;
		} else if (i > mid){
			resdata[pos].value = pdata[j].value;
			resdata[pos++].pos = pdata[j++].pos;
		} else if (pdata[i].value <= pdata[j].value){
			resdata[pos].value = pdata[i].value;
			resdata[pos++].pos = pdata[i++].pos;
		} else {
			resdata[pos].value = pdata[j].value;
			resdata[pos++].pos = pdata[j++].pos;
		}
	}
	for(size_t i = start; i <= end; i++){
		pdata[i].value = resdata[i].value;
		pdata[i].pos = resdata[i].pos;
	}
}
void msortunit(sortunit* data, size_t nvalues){
	sortunit* resdata = calloc(nvalues, sizeof(sortunit));
	for(size_t i = 0; i< nvalues; i++){
		resdata[i].pos = i;
	}
	merge_sortunit(data, resdata, 0, nvalues-1);
	free(resdata);
}
void sort_data(size_t nvalues, int* data, int* copy, PositionVector* pv){
	//qsort_data(data, 0, nvalues-1, pv);	
	//insertion_sort(data, nvalues, pv);
	sortunit* items = calloc(nvalues, sizeof(sortunit));
	for(size_t i = 0; i < nvalues; i++){
		items[i].value = data[i];
		items[i].pos = i;
	}
	msortunit(items, nvalues);
	pv->positions = calloc(nvalues, sizeof(unsigned int));
	if (copy != NULL){
		for(size_t i = 0; i < nvalues; i++){
			copy[i] = items[i].value;
			pv->positions[i] = items[i].pos;
		}
	}
	else {
		for(size_t i = 0; i < nvalues; i++){
			data[i] = items[i].value;
			pv->positions[i] = items[i].pos;
		}
	}
	
	free(items);
}

