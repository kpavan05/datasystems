#ifndef CS165_HASH_TABLE // This is a header guard. It prevents the header from being included more than once.
#define CS165_HASH_TABLE  

typedef int keyType;
typedef int valType;

typedef struct node {
 keyType key;
 valType val;
 struct node *next;
}node;

typedef struct ht_entry {
	node* head;
	int nitems;
}ht_entry;

typedef struct hashtable {
// define the components of the hash table here (e.g. the array, bookkeeping for number of elements, etc)
  int nslots;
  int nassigned;
  ht_entry *table;
} hashtable;


int allocate(hashtable** ht, int size);
int put(hashtable* ht, keyType key, valType value);
int get(hashtable* ht, keyType key, valType *values, int num_values, int* num_results);
int erase(hashtable* ht, keyType key);
int deallocate(hashtable* ht);
int rehash(hashtable *ht);
int add(hashtable *ht, ht_entry *e, node *item);

#endif

