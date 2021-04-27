#ifndef BTREE_H
#define BTREE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define BTREE_ORDER 2048

typedef enum NodeType{
	ROOT,
	LEAF,
	INTERNAL,
	RLEAF
}NodeType;

typedef struct record {
	unsigned int pos;
	struct record* next;
}record;

typedef struct Node{
	int  size;
	int* keys;
	//int* values;
	record** values;
	struct Node* left;
	struct Node* right;
	struct Node** child;
	NodeType type;
}Node;

typedef enum CompareOpType {
    NOCOMPARE = 0,
    COMPARE_EQUAL = 1,
    COMPARE_LESS = 2,
    COMPARE_GREATER = 3,
    COMPARE_LESS_EQUAL = 4,
    COMPARE_GREATER_EQUAL = 5
} CompareOpType;

Node* btree_search(Node* root, int key, CompareOpType optype, int* index);
void btree_insert(Node** node, int key, unsigned int* value);
void btree_delete(Node* root, int key, bool iscluster);
Node* btree_search_node(Node* curnode, int key);
Node* btree_get_parent_node(Node* parent, Node* curnode, int key);
//Node* btree_insert_at(Node* curnode, int key, unsigned int value);
//Node* btree_insert_at_rleaf(Node* curnode, int key, unsigned int value, int index);
//Node* btree_insert_at_internal(Node* curnode, Node* child, int key, int index);

Node* btree_insert_at(Node** root, Node* curnode, int key, unsigned int* value);
Node* btree_insert_at_leaf(Node* curnode, int key, unsigned int* value, int index);
Node* btree_insert_at_internal(Node** root, Node* curnode, Node* child, int key, int index);
Node* btree_insert_at_rleaf(Node** root, Node* curnode, int key, unsigned int* value, int index);

Node* btree_split_leaf_node(Node* curnode);
Node* btree_split_internal_node(Node* curnode);

void btree_increment_values(Node* node, int index);
void btree_decrement_values(Node* node, int index, unsigned int value, bool iscluster);
Node* btree_create_node(NodeType type);
Node* btree_make_root(Node* left, Node* right);
void btree_remove_key(Node* parent, Node* child);
void btree_insert_key(Node* node, Node* child, int key, int index);
void btree_insert_record(Node* node, int key, unsigned int* value, int index);

int binary_search_index(Node* node, int start, int end, int key);
int binary_search_exact(Node* node, int start, int end, int key);
int binary_search_node(Node* node, int start, int end, int key, CompareOpType optype);
int binary_search_data(int* data, int start, int end, int key, CompareOpType optype);
void btree_release(Node* node);
void print_tree(Node* node);
void btree_bulk_insert(Node** root, int* data, unsigned int* positions, unsigned int datalen);
#endif