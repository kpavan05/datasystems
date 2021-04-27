#define _BSD_SOURCE
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include "btree.h"

//Node* root = NULL;

void print_tree(Node* node){
	if (node == NULL) return;
	for(int i =0 ; i < node->size; i++){
		printf("%d," , node->keys[i]);
		if (node->type == LEAF) {
			record* rec = node->values[i];
			while(rec){
				printf("(%d)", rec->pos);
				rec = rec->next;
			}
		}
	}
	printf("\n");
	for(int i= 0; i <= node->size; i++){
		if (node->child)print_tree(node->child[i]);
	}
}
/************************************************************
	B-Tree delete key
*************************************************************/
void btree_delete(Node* root, int key, bool iscluster){
	if (root == NULL) return;

	Node* node = btree_search_node(root, key);
	int index = binary_search_index(node, 0, node->size-1, key);
	unsigned int delpos = node->values[index]->pos;

	free(node->values[index]);
	node->values[index] = NULL;

	for(int i = index; i < node->size-1; i++){
		node->keys[i] = node->keys[i+1];
		node->values[i] = node->values[i+1];
	}
	node->values[node->size-1] = NULL;
	node->size--;

	btree_decrement_values(node, index, delpos, iscluster);

	while (node->size == 0){
		Node* parent = btree_get_parent_node(root, node, key);
		index = binary_search_index(parent, 0, parent->size-1, key);
		for(int i = index; i < parent->size; i++){
			parent->keys[i] = parent->keys[i+1];
			parent->child[i] = parent->child[i+1];
		}
		parent->child[parent->size] = NULL;
		parent->size--;
		free(node);
		node = parent;
	}
}
void btree_decrement_values(Node* node, int index, unsigned int value, bool iscluster){
	Node* curnode = node;
	Node* prevnode = node;
	if (!iscluster){
		while(curnode) {
    		prevnode = curnode;
    		curnode = curnode->left;
    	}
    	curnode = prevnode;	
	}
    int pos = iscluster ? index : 0;
	while (curnode){
		for(int i = pos; i < curnode->size;i++){
			curnode->values[i]->pos -= (curnode->values[i]->pos >= value);
		}
		curnode = curnode->right;
		pos = 0;
	}
}
/************************************************************
	search B-Tree
*************************************************************/
Node* btree_search(Node* root, int key, CompareOpType optype, int* index){
	*index = -1;
	Node* node = btree_search_node(root, key);
	if (node != NULL){
		*index = binary_search_node(node, 0, node->size-1, key, optype);
		return node;
		//if (index == -1) printf("search result not found\n");
		//else printf("search result:%d(%d)\n", node->keys[index], node->values[index]->pos);
	}
	return NULL;
}
// recursive function for searching the key to find the leaf node
Node* btree_search_node(Node* curnode, int key){
	if (curnode->type == LEAF)  return curnode;

	int index = binary_search_index(curnode, 0, curnode->size-1, key);
	return btree_search_node(curnode->child[index], key);
}

// binary search the specific node to find the key (used for btree cluster and btree uncluster)
int binary_search_node(Node* node, int start, int end, int key, CompareOpType optype){
    int mid = start + (end-start)/2;
    if (key >= node->keys[mid] && key < node->keys[mid+1]){
    	switch(optype){
        case COMPARE_LESS_EQUAL:
       	case COMPARE_GREATER_EQUAL:
        	return mid;      
        case COMPARE_LESS:
            return key > node->keys[mid] ? mid : mid-1;
        case COMPARE_GREATER:
            return mid + 1 ;
        default:
            return -1;
    }  
    } 
    if (start >= end) return start;
    if (key < node->keys[mid])
        return binary_search_node(node, start, mid, key, optype);
    else
        return binary_search_node(node, mid+1, end, key, optype);

}
// binary search on the data to find the key (used for sort cluster or sort uncluster)
int binary_search_data(int* data, int start, int end, int key, CompareOpType optype){
    int mid = start + (end-start)/2;

    if (key >= data[mid] && key < data[mid+1]){
    	switch(optype){
        case COMPARE_LESS_EQUAL:
       	case COMPARE_GREATER_EQUAL:
        	return mid;      
        case COMPARE_LESS:
            return key > data[mid] ? mid : mid-1;
        case COMPARE_GREATER:
            return mid + 1 ;
        default:
            return -1;
    }  
    }
     
    if (start >= end) return start;
    if (key <= data[mid])
        return binary_search_data(data, start, mid, key, optype);
    else
        return binary_search_data(data, mid + 1, end, key, optype);

}
// get the parent node of the passed in node (curnode)
Node* btree_get_parent_node(Node* parent, Node* curnode, int key){
	if (parent == NULL || parent == curnode) return NULL;

	int index = binary_search_index(parent, 0, parent->size-1, key);
	if (parent->child[index] == curnode) 
		return parent;
	else 
		return btree_get_parent_node(parent->child[index], curnode, key);
}
/***************************************************************
	B-Tree insert
****************************************************************/
void btree_insert(Node** root, int key, unsigned int* value){
	if (*root == NULL){
		*root = btree_create_node(RLEAF);
		btree_insert_record(*root, key, value,0);
		return;
		//root->values[0] = value;
		//root->keys[0] = key;		
		//root->size++;;		
		
	} else {
		btree_insert_at(root, *root, key, value);		
	}
}

// make a new root once the original root is full
Node* btree_make_root(Node* left, Node* right){
	Node* node = btree_create_node(ROOT);
	node->keys[0] = right->keys[0];
	if (right->type == INTERNAL){
		btree_remove_key(node, right);
		left->type = INTERNAL;		
	}
	node->child[0] = left;
	node->child[1] = right;
	node->size = 1;
	return node;
}
// recursive backtracking to insert the key from leaf node
Node* btree_insert_at(Node** root, Node* curnode, int key, unsigned int* value){
	if (curnode == NULL) return NULL;

	int index = binary_search_index(curnode, 0, curnode->size-1, key);
	if (curnode->type == LEAF ){	
		return btree_insert_at_leaf(curnode, key, value, index);
	}
	if (curnode->type == RLEAF){
		return btree_insert_at_rleaf(root, curnode, key, value, index);
	}

	Node* child = index <= curnode->size ? curnode->child[index] : NULL;
	Node* newchild = child != NULL ? btree_insert_at(root, child, key, value): NULL;
	if (newchild) {
		return btree_insert_at_internal(root, curnode, newchild, key, index);
	}	
	return NULL;
}
// insert into internal node
Node* btree_insert_at_internal(Node** root, Node* curnode, Node* child, int key, int index){
	if (curnode->size < BTREE_ORDER - 1){
		btree_insert_key(curnode, child, key, index);
		btree_remove_key(curnode, child);
		return NULL;
	}
	Node* node = btree_split_internal_node(curnode);
	if (child) {
		if (child->keys[0] < node->keys[0] )
			btree_insert_at_internal(root, curnode, child, key, index);
		else
			btree_insert_at_internal(root, node, child, key, index - curnode->size);
	}
	if (curnode->type == ROOT){
		Node* newroot = btree_make_root(curnode, node);
		*root = newroot;
		return NULL;
	}
	return node;
}
// insert into leaf node
Node* btree_insert_at_leaf(Node* curnode, int key, unsigned int* value, int index){
	if (curnode->size < BTREE_ORDER - 1){
		btree_insert_record(curnode, key, value, index);
		return NULL;
	}
	Node* node = btree_split_leaf_node(curnode);
	if (key < node->keys[0] ){
		btree_insert_at_leaf(curnode, key, value, index);
	}
	else {
		btree_insert_at_leaf(node, key, value, index-curnode->size);
	}
	return node;
}
// insert into combined root leaf node (transitional node)
// this type exists when number of elements are less than order of tree
Node* btree_insert_at_rleaf(Node** root, Node* curnode, int key, unsigned int* value, int index){
	if (curnode->size < BTREE_ORDER - 1){
		btree_insert_record(curnode, key, value, index);
		return NULL;
	}
	Node* node = btree_split_leaf_node(curnode);
	if (key < node->keys[0]){
		btree_insert_at_rleaf(root, curnode, key, value, index);
	} else {
		btree_insert_at_rleaf(root, node, key, value, index - curnode->size);
	}
	Node* newroot = btree_make_root(curnode, node);
	*root = newroot;
	return NULL;
}

/****************************************************************
	utility functions for B-Tree
****************************************************************/
// split the internal node into two and move half of its contents
Node* btree_split_internal_node(Node* curnode){
	int split = BTREE_ORDER/2;	   	
	Node* newnode = btree_create_node(INTERNAL);

	newnode->child[curnode->size-split] = curnode->child[curnode->size];
	curnode->child[curnode->size] = NULL;
	for(int i = curnode->size-1; i > split; i--){
		newnode->keys[i-split] = curnode->keys[i];
		newnode->child[i-split] = curnode->child[i];
		curnode->child[i] = NULL;
		newnode->size++;
	}
	newnode->keys[0] = curnode->keys[split];
	newnode->child[0] = curnode->child[split];
	newnode->size++;
   	curnode->size = split;
   	return newnode;	   
}
// split the leaf/root node into two and move half of its contents
Node* btree_split_leaf_node(Node* curnode){
	int split = BTREE_ORDER/2;
	Node* newnode = btree_create_node(LEAF);
	
   	for(int i = split; i < curnode->size; i++) {
   		newnode->keys[i-split] = curnode->keys[i];
   		newnode->values[i-split] = curnode->values[i];
   		newnode->size++; 	
   	}
   	newnode->left  = curnode;
   	newnode->right = curnode->right;
   	curnode->right = newnode;
   	curnode->size = split;
   	curnode->type = LEAF;
   	return newnode;	   
} 
void btree_insert_record(Node* node, int key, unsigned int* value, int index){
	if (node->size == 0) {
		node->keys[index] = key;
		node->values[index] = NULL;
		node->size = 1;
	}
	if (node->keys[index] != key){
		for(int i = node->size; i > index; i--){
			node->keys[i] = node->keys[i-1];
			node->values[i] = node->values[i-1];
		}
		node->size = node->keys[index] != key ? node->size+1: node->size;
		node->keys[index] = key;
		node->values[index] = NULL;
	}
	//node->values[index] = value;
	if (node->values[index] == NULL){
		node->values[index] = malloc(sizeof(record));
		if (*value == UINT_MAX){
			*value = index == 0 ? node->values[index+1]->pos: node->values[index-1]->pos +1;
			node->values[index]->pos = *value;		
		} else {
			node->values[index]->pos = *value;
		}
		btree_increment_values(node, index);		
		node->values[index]->next = NULL;
	} else {
		record* rec = malloc(sizeof(record));
		rec->pos = *value;
		rec->next = NULL;

		record* next = node->values[index];
		record* prev = node->values[index];
		while(next){
			prev = next;
			next = next->next;
		}
		if (prev) prev->next = rec;
	}		
}
void btree_increment_values(Node* node, int index){
	Node* curnode = node;
	int curpos = index + 1;
	unsigned int value = node->values[index]->pos;
	while (curnode){
		for(int i = curpos; i < curnode->size;i++){
			curnode->values[i]->pos += (curnode->values[i]->pos >= value);
		}
		curnode = curnode->right;
		curpos = 0;
	}
}

// insert key into internal node
void btree_insert_key(Node* node, Node* child, int key, int index){
	(void)key;
	for(int i = node->size; i > index; i--){
		node->keys[i] = node->keys[i-1];
		node->child[i+1] = node->child[i];
	}
	if (child){
		node->keys[index] = child->keys[0];
		node->child[index+1] = child;
	} 
	node->size++;
}
// remove key and its child pointer from internal node when key moves to parent node.
void btree_remove_key(Node* parent, Node* cnode){
	(void)parent;
	if (!cnode ||  cnode->type != INTERNAL) return;

	cnode->child[0] = NULL;
	for(int i= 0; i < cnode->size-1; i++){
		cnode->keys[i] = cnode->keys[i+1];
		cnode->child[i] = cnode->child[i+1];
	}
	cnode->child[cnode->size-1] = cnode->child[cnode->size];
	cnode->size--;
}
// binary search to find the child node
int binary_search_index(Node* node, int start, int end, int key){	
	if (node == NULL) return 0;
	if (key < node->keys[0]) return 0;

	int index = 0;
	for(int i = end; i >= start; i--){
		if (key > node->keys[i]){
			index = i + 1; break;
		}
	}
	return index;
}
// binary search to find the key
int binary_search_exact(Node* node, int start, int end, int key){
	int mid = start + (end-start)/2;
	if (key == node->keys[mid]) return mid;
	if (start >= end) return -1;

	if (key < node->keys[mid])
		return binary_search_exact(node, start, mid, key);
	else
		return binary_search_exact(node, mid+1, end, key);

}
// handy function to create node
Node* btree_create_node(NodeType type){
	Node* node = malloc(sizeof(Node));
	node->keys = calloc(BTREE_ORDER, sizeof(int));
	//node->values = type == LEAF || type == RLEAF? calloc(BTREE_ORDER, sizeof(int)) : NULL;
	node->values = type == LEAF || type == RLEAF? calloc(BTREE_ORDER, sizeof(record*)) : NULL;
	node->child = type == LEAF || type == RLEAF ? NULL : calloc(BTREE_ORDER + 1, sizeof(Node*));
	node->left = NULL;
	node->right = NULL;
	node->type = type;
	node->size = 0;
	return node;
}

void btree_release(Node* node){
	if (node->type == LEAF) {
		for(int i =0 ; i < node->size; i++){	
			free(node->values[i]);	
		}
		return;
	}
	for(int i= 0; i <= node->size; i++){
		btree_release(node->child[i]);
		free(node->child[i]);
	}
	if (node->type != ROOT) free(node);
}

Node* btree_create_leaf_nodes(int* data, unsigned int* positions, unsigned int datalen, unsigned int* num_nodes){
	Node* curnode = NULL;
	Node* prevnode = NULL;
	Node* startnode = NULL;
	unsigned int posvalue = 0;
	int count = 0;
	
	*num_nodes = 0;
	int vec_size = BTREE_ORDER/2;

	for (unsigned int i = 0; i < datalen; i+= vec_size){
		curnode = btree_create_node(LEAF);
		if (!startnode) startnode = curnode;

		count = 0;
		for(unsigned int j = i ; j < i+ vec_size; j++){
			curnode->keys[count] = data[j];
			posvalue = positions[j];
			btree_insert_record(curnode, data[j], &posvalue, count);
			count++;
		}
		curnode->size = count;
		curnode->left = prevnode;
		if (prevnode) prevnode->right = curnode;
		*num_nodes = *num_nodes + 1;
		prevnode = curnode;
	}

	
	return startnode;
}

// split the internal node into two and move half of its contents
Node* btree_split_internal_node1(Node* curnode, bool isleft){
	int split = isleft ? BTREE_ORDER/2 : BTREE_ORDER/2 + 1;	   	
	Node* newnode = btree_create_node(INTERNAL);

	newnode->child[curnode->size-split] = curnode->child[curnode->size];
	
	for(int i = curnode->size-1; i > split; i--){
		newnode->keys[i-split] = curnode->keys[i];
		newnode->child[i-split] = curnode->child[i];
		curnode->child[i] = NULL;
		newnode->size++;
	}
	if (curnode->size > split){
		curnode->child[curnode->size] = NULL;
		newnode->keys[0] = curnode->keys[split];
		newnode->child[0] = curnode->child[split];
		newnode->size++;
   		curnode->size = split;
	}
	
   	return newnode;	   
}

Node* btree_create_internal_nodes(Node* child, unsigned int numnodes, int level, unsigned int* ninternals){
	Node* node = btree_create_node(INTERNAL);
	node->child[0] = child;
	Node* start = node;
	Node* newnode = NULL;
	bool isleft = true;

	*ninternals = 1;
	for(unsigned int i = 1; i < numnodes && child; i++){
		if (node->size == BTREE_ORDER -1) {
			newnode = btree_split_internal_node1(node, isleft);
			node->right = newnode;
			newnode->left = node;
			node = newnode;
			*ninternals = *ninternals + 1;
			isleft = false;
		}
		child = child->right;
		node->keys[node->size++] = child->keys[0];
		if (level != 0){
			btree_remove_key(node, child);			
		} 
		node->child[node->size] = child;
	}
	/*
	printf("level:%d\n", level);
	Node* prev = start;
	Node* next = prev;
	while(next){
		for(unsigned int i = 0; i < next->size; i++){
			printf("%d,", next->keys[i]);
		}
		printf("\n");
		next = next->right;
	}
	*/
	return start;
}
void btree_bulk_insert(Node** root, int* data, unsigned int* positions, unsigned int datalen){
	unsigned int numnodes = 0;
	unsigned int ninternals = 0;
	int level = 0;
	Node* l_node = NULL;
	Node* child = btree_create_leaf_nodes(data, positions, datalen, &numnodes);
	
	while(ninternals != 1){
		l_node = btree_create_internal_nodes(child, numnodes, level, &ninternals);
		child = l_node;
		numnodes = ninternals;
		level++;
	}
	l_node->type = ROOT;
	*root = l_node;

}
