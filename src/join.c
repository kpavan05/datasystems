#define _BSD_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/time.h>
#include "utils.h"
#include "join.h"
#include "hash_table.h"

void db_nested_join_i(int* payload1, int* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2){
    size_t capacity, cursize;
    size_t ntuples1 = s1->num_tuples;
    size_t ntuples2 = s2->num_tuples;
    //int bval;
    cursize = 0;
    capacity = ntuples1;
    r1->positions = calloc(capacity, sizeof(unsigned int));
    r2->positions = calloc(capacity, sizeof(unsigned int));
    struct timeval st, et;
    gettimeofday(&st,NULL);
    for(size_t i = 0; i < ntuples1; i++){
        /*if (cursize + ntuples2 > capacity){
            capacity *= 2;
            r1->positions = realloc(r1->positions, capacity*sizeof(unsigned int));
            r2->positions = realloc(r2->positions, capacity*sizeof(unsigned int));
        }*/
        for(size_t j = 0; j < ntuples2; j++){
            if (payload1[i] == payload2[j]) {
                r1->positions[cursize] = s1->positions[i];
                r2->positions[cursize] = s2->positions[j];
                cursize++;
            }
            /*          
            bval = (payload1[i] == payload2[j]);
            r1->positions[cursize] = s1->positions[i] & ~(0xFFFFFFFF + bval);
            r2->positions[cursize] = s2->positions[j] & ~(0xFFFFFFFF + bval);
            cursize += bval;
            */          
        }
    }
    r1->num_tuples = cursize;
    r2->num_tuples = cursize;
    gettimeofday(&et,NULL);
    long elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    cs165_log(stdout, "--Time taken for bulk insertion run %ld usec\n", elapsed);
}

void db_block_nested_join_i(int* payload1, int* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2){
    size_t capacity, cursize;
    size_t ntuples1 = s1->num_tuples;
    size_t ntuples2 = s2->num_tuples;
    int bval;
    cursize = 0;
    capacity = ntuples1;
    r1->positions = calloc(capacity, sizeof(unsigned int));
    r2->positions = calloc(capacity, sizeof(unsigned int));
    size_t vec_size = 512;

    struct timeval st, et;
    gettimeofday(&st,NULL);

    for(size_t i = 0; i < ntuples1; i+= vec_size){
        if (cursize + ntuples2 > capacity){
            capacity *= 2;
            r1->positions = realloc(r1->positions, capacity*sizeof(unsigned int));
            r2->positions = realloc(r2->positions, capacity*sizeof(unsigned int));
        }
        for(size_t j = 0; j < ntuples2; j+= vec_size){  
            size_t klimit =  i+vec_size < ntuples1 ? i+vec_size : ntuples1;         
            for(size_t k = i; k < klimit; k++){
                size_t mlimit = j+vec_size < ntuples2 ? j+vec_size : ntuples2;
                for(size_t m = j; m < mlimit; m++){
                    bval = (payload1[k] == payload2[m]);
                    r1->positions[cursize] = s1->positions[k] & ~(0xFFFFFFFF + bval);
                    r2->positions[cursize] = s2->positions[m] & ~(0xFFFFFFFF + bval);
                    cursize += bval;
                }
            }
        }
    }
    r1->num_tuples = cursize;
    r2->num_tuples = cursize;
    gettimeofday(&et,NULL);
    long elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    cs165_log(stdout, "--Time taken for bulk insertion run %ld usec\n", elapsed);
}

void db_sort_join_i(int* payload1, int* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2){
    size_t capacity, cursize;
    size_t ntuples1 = s1->num_tuples;
    size_t ntuples2 = s2->num_tuples;
    size_t i = 0, j = 0;
    cursize = 0;
    capacity = ntuples1;
    r1->positions = calloc(capacity, sizeof(unsigned int));
    r2->positions = calloc(capacity, sizeof(unsigned int));
    struct timeval st, et;
    gettimeofday(&st,NULL);

    while (i < ntuples1 || j < ntuples2){
        if (payload1[i] < payload2[j]) i++;
        else if (payload1[i] > payload2[j]) j++;
        else {
            if (cursize == capacity){
                capacity *= 2;
                r1->positions = realloc(r1->positions, capacity*sizeof(unsigned int));
                r2->positions = realloc(r2->positions, capacity*sizeof(unsigned int));
            }
            r1->positions[cursize] = s1->positions[i++];
            r2->positions[cursize++] = s2->positions[j++];
        }
    }
    r1->num_tuples = cursize;
    r2->num_tuples = cursize;
    gettimeofday(&et,NULL);
    long elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    cs165_log(stdout, "--Time taken for sort merge run %ld usec\n", elapsed);
}
void db_nested_join_l(long* payload1, long* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2){
    size_t capacity, cursize;
    size_t ntuples1 = s1->num_tuples;
    size_t ntuples2 = s2->num_tuples;
    bool bval;
    cursize = 0;
    capacity = ntuples1;
    r1->positions = calloc(capacity, sizeof(unsigned int));
    r2->positions = calloc(capacity, sizeof(unsigned int));
    
    for(size_t i = 0; i < ntuples1; i++){
        if (cursize + ntuples2 > capacity){
            capacity *= 2;
            r1->positions = realloc(r1->positions, capacity*sizeof(unsigned int));
            r2->positions = realloc(r2->positions, capacity*sizeof(unsigned int));
        }
        for(size_t j = 0; j < ntuples2; j++){
            bval = (payload1[i] == payload2[j]);
            r1->positions[cursize] = s1->positions[i] & ~(0xFFFFFFFF + bval);
            r2->positions[cursize] = s2->positions[j] & ~(0xFFFFFFFF + bval);
            cursize += bval;
        }
    }
    r1->num_tuples = cursize;
    r2->num_tuples = cursize;
}

void db_hash_join_i(int* payload1, int* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2){
    size_t count = 0, capacity = 0;
    size_t ntuples1 = s1->num_tuples;
    size_t ntuples2 = s2->num_tuples;
    int retval = 0;
    int nresults = 0;
    int val_size = 1;
    
    hashtable *ht = NULL;
    int* values = calloc(val_size, sizeof(int));

    r1->positions = calloc(ntuples1, sizeof(unsigned int));
    r2->positions = calloc(ntuples1, sizeof(unsigned int));
    capacity = ntuples1;
    /*
    cs165_log(stdout,"printing p1, s1\n");
    for(size_t i = 0; i < ntuples1; i++){
        cs165_log(stdout, "p1:%d , s1: %u\n", payload1[i], s1->positions[i]);
    }
    cs165_log(stdout,"printing p2, s2\n");
    for(size_t i = 0; i < ntuples2; i++){
        cs165_log(stdout, "p2: %d, s2: %u\n", payload2[i], s2->positions[i]);
    }
    */
    allocate(&ht, ntuples1);
    for(size_t i = 0; i < ntuples1; i++){
        put(ht, payload1[i], s1->positions[i]);
    }

    for(size_t i = 0; i < ntuples2; i++){
        retval = get(ht, payload2[i], values, val_size, &nresults);
        if ( retval != 0) continue;
        if (nresults > val_size){
            values = realloc(values, nresults*sizeof(int));
            get(ht, payload2[i], values, val_size, &nresults);
        }
        if (val_size == 1) {
            r1->positions[count] = values[0];
            r2->positions[count++] = s2->positions[i];
        } else {
            for(int j = 0; j < nresults; j++){
                r1->positions[count] = values[j];
                r2->positions[count++] = s2->positions[i];
            }
        } 
        if (count + (ntuples2-i) > capacity){
            capacity *= 2;
            r1->positions = realloc(r1->positions, capacity*sizeof(unsigned int));
            r2->positions = realloc(r2->positions, capacity*sizeof(unsigned int));
        }                        
    }
    r1->num_tuples = count;
    r2->num_tuples = count;
    deallocate(ht);
    free(values);
    /*
    for(size_t k = 0; k < count; k++){
        printf("t1: %u , t2: %u\n", r1->positions[k], r2->positions[k]);
    }
    */
}


void db_grace_join_i(int* payload1, int* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2){
    size_t count = 0, capacity = 0;
    size_t ntuples1 = s1->num_tuples;
    size_t ntuples2 = s2->num_tuples;
    int retval = 0;
    int nresults = 0;
    int val_size = 1;
    
    int* values = calloc(val_size, sizeof(int));

    r1->positions = calloc(ntuples1, sizeof(unsigned int));
    r2->positions = calloc(ntuples1, sizeof(unsigned int));
    capacity = ntuples1;
    
    hashtable *ht1 = NULL;
    hashtable *ht2 = NULL;
    //partition join input 1
    allocate(&ht1, MAX_PARTITIONS);
    for(size_t i = 0; i < ntuples1; i++){
        put(ht1, payload1[i], s1->positions[i]);
    }
    //partition join input 2
    allocate(&ht2, MAX_PARTITIONS);
    for(size_t i = 0; i < ntuples2; i++){
        put(ht2, payload2[i], s2->positions[i]);
    }

    hashtable *curht = NULL;
    hnode* head = NULL;
    hnode* tohash = NULL;
    hnode* curnode1 = NULL;
    hnode* curnode2 = NULL;
    int nitems1, nitems2;
    int curkey, curvalue;
    int bval = 0;
    int npartitions = ht1->nslots;

    for(int i = 0; i < npartitions; i++){
        nitems1 = ht1->table[i].nitems;
        nitems2 = ht2->table[i].nitems;

        if (nitems1 == 0 || nitems2 == 0) continue;
        if (count + nitems1 > capacity){
            capacity *= 2;
            r1->positions = realloc(r1->positions, capacity*sizeof(unsigned int));
            r2->positions = realloc(r2->positions, capacity*sizeof(unsigned int));
        } 

        curht = NULL;
        bval = nitems1 > nitems2;
        nitems1 > nitems2 ? allocate(&curht, nitems2) : allocate(&curht, nitems1);
        head =  nitems1 > nitems2 ? ht2->table[i].head : ht1->table[i].head;
        tohash = nitems1 > nitems2 ? ht1->table[i].head : ht2->table[i].head;
        
        curnode1 = head;
        while(curnode1){
            put(curht, curnode1->key, curnode1->val);
            curnode1 = curnode1->next;
        }

        curnode2 = tohash;
        while(curnode2){
            curkey = curnode2->key;
            curvalue = curnode2->val;
            nresults = 0;
            retval = get(curht, curkey, values, val_size, &nresults);
            if ( retval != 0) continue;
            if (nresults > val_size){
                values = realloc(values, nresults*sizeof(int));
                get(curht, curkey, values, val_size, &nresults);
                val_size = nresults;
            }
            
            if (nresults == 1) {
                r1->positions[count] =  bval*curvalue + values[0]*(~bval);
                r2->positions[count++] = bval*values[0] + curvalue*(~bval); 
            } else {
                for(int j = 0; j < nresults; j++){                    
                    r1->positions[count] =  bval*curvalue + values[j]*(~bval);
                    r2->positions[count++] = bval*values[j] + curvalue*(~bval); 
                }
            } 
            curnode2 = curnode2->next;                       
        }
        deallocate(curht);        
    }
    r1->num_tuples = count;
    r2->num_tuples = count;
    deallocate(ht1);
    deallocate(ht2); 
    free(values);
    for(size_t k = 0; k < count; k++){
        printf("t1: %u , t2: %u\n", r1->positions[k], r2->positions[k]);
    }
}


void db_hash_join_l(long* payload1, long* payload2, PositionVector* s1, PositionVector* s2, PositionVector* r1, PositionVector* r2){
    (void)payload1, (void)payload2, (void)s1, (void)s2, (void)r1, (void)r2;
    /*
    size_t capacity, cursize;
    size_t ntuples1 = s1->num_tuples;
    size_t ntuples2 = s2->num_tuples;
    bool bval;
    cursize = 0;
    capacity = ntuples1;
    r1->positions = calloc(capacity, sizeof(unsigned int));
    r2->positions = calloc(capacity, sizeof(unsigned int));
    
    for(size_t i = 0; i < ntuples1; i++){
        
    }
    r1->num_tuples = cursize;
    r2->num_tuples = cursize;
    */
}