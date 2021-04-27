#include <stdlib.h>
#include <stdio.h>
#include "hash_table.h"
#include <assert.h>


// Initialize the components of a hashtable.
// The size parameter is the expected number of elements to be inserted.
// This method returns an error code, 0 for success and -1 otherwise (e.g., if the parameter passed to the method is not null, if malloc fails, etc).
int allocate(hashtable** ht, int size) {
    if (*ht != NULL) return -1;

    int exp = 0, nslots = 0;
    hashtable *pht = NULL;
   
    pht = malloc(sizeof(hashtable));
    assert(pht != NULL);
    while(size != 0){       
       size = size >>1;
       exp++;
    }

    nslots = (1 << exp) -1;
    pht->table = (ht_entry*)calloc(nslots, sizeof(ht_entry));
    assert(pht->table != NULL);

    pht->nassigned = 0;
    pht->nslots = nslots;

    for(int i = 0; i < nslots; i++){
       pht->table[i].head = NULL;
       pht->table[i].nitems = 0;
    }
      
    *ht = pht;
    return 0;
    
}

// This method inserts a key-value pair into the hash table.
// It returns an error code, 0 for success and -1 otherwise (e.g., if malloc is called and fails).
int put(hashtable* ht, keyType key, valType value) {    
    node *item = NULL;    
    item = malloc(sizeof(node));
    assert(item != NULL);

    item->key = key;
    item->val = value;
    item->next = NULL;
    
    assert(add(ht, ht->table, item) == 0);
    
    /* functionality for rehashing in case number of collisions >5
    int index = -1;
    double nthreshold = 5;  // hardcoding the allowed number of collisions as 5 
    index = key % ht->nslots;
    if (ht->table[index].nitems > nthreshold) {
        printf("start reallocating hash table for index:%d\n", index);
        assert(rehash(ht) == 0);
        printf("done reallocating hash table for index:%d\n", index);
    }
    */
    return 0;
}

// This method retrieves entries with a matching key and stores the corresponding values in the
// values array. The size of the values array is given by the parameter
// num_values. If there are more matching entries than num_values, they are not
// stored in the values array to avoid a buffer overflow. The function returns
// the number of matching entries using the num_results pointer. If the value of num_results is greater than
// num_values, the caller can invoke this function again (with a larger buffer)
// to get values that it missed during the first call. 
// This method returns an error code, 0 for success and -1 otherwise (e.g., if the hashtable is not allocated).
int get(hashtable* ht, keyType key, valType *values, int num_values, int* num_results) {
    node *next = NULL;
    int count = 0;
    int index = -1;

    if (ht == NULL) return -1;

    index =  key % ht->nslots;
    next = ht->table[index].head;
    while(next){
      count = count + (next->key == key );  
      next = next->next;
    }
    
    *num_results = count;
    if (num_values < count) return 0;

    count = 0;
    next = ht->table[index].head;
    while(next){
      if (next->key == key)values[count++] = next->val;
      //printf("retreived at index:%d, key:%d, value:%d\n", index, next->key, next->val);
      next = next->next;
    }

    return 0;
}

// This method erases all key-value pairs with a given key from the hash table.
// It returns an error code, 0 for success and -1 otherwise (e.g., if the hashtable is not allocated).
int erase(hashtable* ht, keyType key) {
    int index = -1;
    node *next = NULL;
    node *prev = NULL;

    index = key % ht->nslots;
    next = ht->table[index].head;

    while(next){
        if (next->key == key){
           if (prev) prev->next = next->next;
           else ht->table[index].head = next->next;
           ht->table[index].nitems--;
           next->next = NULL;
           free(next);
           break;
        }    
        prev = next;
        next = next->next;
    }
    return 0;
}

// This method frees all memory occupied by the hash table.
// It returns an error code, 0 for success and -1 otherwise.
int deallocate(hashtable* ht) {
    node *item = NULL;
    int nitems = 0;

    if (ht == NULL) return -1;

    int size = ht->nslots - 1;
    
    while(size >= 0){
        nitems = ht->table[size].nitems;
        for (int j = 0 ; j < nitems; j++){
            item = ht->table[size].head;
            ht->table[size].head = ht->table[size].head->next;
            free(item);
        }
        ht->table[size].head = NULL;
        size--;
    }
    free(ht->table);
    free(ht);
    return 0;
}

/*
helper function to add items to linked list 
called from both put and rehash function
*/
int add(hashtable *ht, ht_entry *table, node *item){
    int index = 0 ;
    node *next = NULL;
    node *prev = NULL;
    
    index = item->key % ht->nslots;
    next = table[index].head;
    while (next){
        prev = next;
        next = next->next;
    }

    if (prev){
        prev->next = item;    
    }
    else{
        table[index].head = item;
        ht->nassigned++;
    } 
    table[index].nitems++;
    //printf("inserted at index:%d, key:%d, value:%d\n", index, item->key, item->val);
    return 0;
}

int rehash(hashtable *ht){
  node *next = NULL;
  int numslots = ht->nslots;

  ht->nassigned = 0;  
  ht_entry *oldarr = ht->table;  
  ht->nslots = 2*ht->nslots - 1;

  ht_entry *newarr = (ht_entry*)calloc(ht->nslots, sizeof(ht_entry));
  assert(newarr != NULL);

  newarr->head = NULL;
  newarr->nitems = 0;

  for(int i = 0; i < numslots; i++){
    if (oldarr[i].head == NULL) continue;
    next = oldarr[i].head;
    while(next){
      assert(add(ht, newarr, next) == 0);
      next = next->next;
    }
    oldarr[i].head = NULL;
  }

  free(ht->table);
  ht->table = newarr;
  return 0;
}
