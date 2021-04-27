#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include "common.h"
#include "cs165_api.h"
#include "message.h"
#include "utils.h"
#include "btree.h"
#include "ioutils.h"

#define MAX_PATH 128
/*
  catalog file is named .dbcatalog located under home folder.
  this stores all the meta data for the database
  i.e. db name and tables under it and columns under each table
*/
FILE* open_catalog(char* mode){
    FILE *fp = NULL;
    /*
    char cat_file[MAX_PATH] = "";
    char *homedir = getenv("HOME");
    
    if (homedir == NULL) {   
        return NULL;
    } 
    sprintf(cat_file, "%s/%s", homedir, "dbcatalog");
    fp = fopen(cat_file, mode);
    */
    fp = fopen("dbcatalog", mode);
    return fp;
}
/* 
  create or open a folder for persistence 
  database is the main folder under home folder
  inside this is the folder for each table
     
*/
int open_db_folder(char *name, char *parentname, bool create){    
    char fullpath[MAX_PATH] = "";
    /*
    char *homedir = getenv("HOME");
    if (homedir != NULL) {
        if (parentname == NULL)
            sprintf(fullpath, "%s/%s", homedir, name);
        else
            sprintf(fullpath, "%s/%s/%s", homedir, parentname, name);
    }
    */
    if (parentname == NULL)
        sprintf(fullpath, "%s", name);
    else
        sprintf(fullpath, "%s/%s", parentname, name);

    DIR *dir = opendir(fullpath);
    if (dir != NULL) {
        closedir(dir);
        return 0;
    }
    if (create) {
        return mkdir(fullpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
    cs165_log(stdout, "directory %s cannot be created.", name);
    /*
    DIR *dir  = NULL;
    struct dirent *entry = NULL;
    while( (entry = readdir(dir)) != NULL){
        if (entry->d_type == DT_DIR){
            ret_status = commit_table(db, entry->d_name);
        }
    }
    */  
    return -1;
}
/*
  create or open a file on disk for persistence
  each column is stored as a separate file.
  opens the column file on the disk based on the mode
*/
FILE* open_db_file(char *dbname, char *tblname, char* filename, char* mode){ 
    char fullpath[MAX_PATH] = "";
    /*
    char *homedir = getenv("HOME");
    if (homedir != NULL) {
        sprintf(fullpath, "%s/%s/%s/%s", homedir, dbname, tblname, filename);
        return fopen(fullpath, mode);
    }
    */
    sprintf(fullpath, "%s/%s/%s", dbname, tblname, filename);
    return fopen(fullpath, mode);
    return NULL;
}
/*
  helper function to read the catalog file and retreive the metadata of
  database. this function instantiates and populates the Db, Table and Column
  data structures 
*/
void open_db_helper(char *buf, ClientContext* context, Status *ret_status){
    Table *table = NULL;
    Column *column = NULL;
    int token_status;
    int num_columns;
    int num_items;
    char *token;
    char *endptr;
    char tbl_name[MAX_SIZE_NAME];

    buf = trim_newline(buf);
    token = next_token(&buf, COLON, &token_status);
    
    if (strncmp(token, "Dbname", 6) == 0){
        *ret_status = db_add(buf, false);
    }
    else if (strncmp(token, "Table", 5) == 0){
        buf = trim_whitespace(buf);
        token = next_token(&buf, COMMA, &token_status);
        strcpy(tbl_name, token);
        token = next_token(&buf, COMMA, &token_status);
        num_columns = strtol(token, &endptr, 10);
        token = next_token(&buf, COMMA, &token_status);
        num_items = strtol(token, &endptr, 10);
        table = db_create_table(current_db, tbl_name, num_columns, ret_status);
        table->table_length = num_items; 
    }
    else if (strncmp(token, "Column", 6) == 0){
        table = &current_db->tables[current_db->tables_size - 1];

        int ntokens = find_num_tokens(buf, ',');
        if (ntokens == 2) {
            char* colname = next_token(&buf, COMMA, &token_status);
            char* cidxtype = next_token(&buf, COMMA, &token_status);;
            int idxtype = strtol(cidxtype, &endptr, 10);

            column = db_create_column(colname, table, false, ret_status);
            sprintf(column->full_name, "%s.%s.%s", current_db->name, table->name, column->name);
            add_base_column_catalog_manager(column->full_name, column, context);
            read_column_data(column, table, ret_status);
            db_initialize_index(column, idxtype);
            current_db->hasindex = true;

        } else{
            column = db_create_column(buf, table, false, ret_status);
            sprintf(column->full_name, "%s.%s.%s", current_db->name, table->name, column->name);
            add_base_column_catalog_manager(column->full_name, column, context);
            read_column_data(column, table, ret_status);
        }
    }
}
/*
  load the data in each column store file into memory
  i.e. put into Column struct.
*/
void read_column_data(Column *column, Table *table, Status *ret_status){
    //char line[MAX_DATA_LINE];
    //char *endptr;
    //long val;
    //size_t nitems = 0;
    FILE *fpcol = open_db_file(current_db->name, table->name, column->name, "r");
    if (fpcol == NULL){
      ret_status->code = ERROR;
      return;
    }
    fread(column->data, sizeof(int), table->table_length, fpcol);
    column->col_size = table->table_length;
    /*
    while(fgets(line, MAX_DATA_LINE, fpcol) != NULL){
        val = strtol(line, &endptr, 10);
        //val = atoi(line);
        column->data[nitems++] = val;
    }

    column->col_size = nitems;
    */
    fclose(fpcol);
    fpcol = NULL;
}

/*
  delete database folder and all the column store files under it
*/
void delete_db_folder(char *name){
    (void)name;
}
/*
 delete catalog file
*/
void delete_db_catalog(){
    /*
    char cat_file[MAX_PATH];
    sprintf(cat_file, "%s/dbcatalog", getenv("HOME"));
    sprintf(cat_file, "%dbcatalog", getenv("HOME"));
    remove(cat_file);
    */
    remove("dbcatalog");
}

void commit_column(Table* table, Column *column){
    //char data[MAX_DATA_LINE];
    FILE *fpcol = open_db_file(current_db->name, table->name, column->name, "wb");
    if (fpcol == NULL) return;
    
    //if (!column->isdirty) return;
    int nrows = table->table_length;
    fwrite(column->data, sizeof(int), nrows, fpcol);
    /*
    for(int i= 0; i < nrows; i++){
        sprintf(data, "%d\n", column->data[i]);
        fputs(data, fpcol);
    }
    */
    fflush(fpcol);
    fclose(fpcol);
    fpcol = NULL;
    
    if (column->index) {
        commit_index(table, column);
        free(column->index);
    }
    free(column->data);
    column->data = NULL;
    // write the data to disk
}

void commit_table(FILE* fpcatalog, Table *table){
    char data[MAX_DATA_LINE];
    int ncolumns = table->col_count;

    for(int i = 0; i < ncolumns; i++){
        if (table->columns[i].index != NULL){
            sprintf(data, "Column:%s,%d\n", table->columns[i].name, table->columns[i].index->type);
        } else {
            sprintf(data, "Column:%s\n", table->columns[i].name);
        }
        fputs(data, fpcatalog);
        commit_column(table, &table->columns[i]);
    }   
    free(table->columns);
    table->columns = NULL;
}



char newline      = '\n';
char startnode[]  = "<Node>\n";
char endnode[]    = "</Node>\n";
char startkeys[]  = "<Keys>\n";
char endkeys[]    = "</Keys>\n";
char startchild[] = "<child>\n";    
char endchild[]   = "</child>\n";
char startval[]   = "<values>\n";
char endval[]     = "</values>\n";

void commit_btree(FILE* fp, Node* node, int level){
    int type = node->type;
    int count = 0;
    //fwrite(startnode, sizeof(char),strlen(startnode), fp);
    fwrite(&level, sizeof(int), 1, fp);
    fwrite(&node->size, sizeof(int), 1, fp);
    fwrite(&type, sizeof(int), 1, fp);
    fwrite(&newline, sizeof(char),1, fp);
    
    //fwrite(startkeys, sizeof(char),strlen(startkeys), fp);
    fwrite(node->keys, sizeof(int), node->size, fp);
    fwrite(&newline, sizeof(char),1, fp);
    //fwrite(endkeys, sizeof(char),strlen(endkeys), fp);
    
    if (node->type == LEAF || node->type == RLEAF) {
        //fwrite(startval, sizeof(char),strlen(startval), fp);
        
        for(int j = 0; j < node->size; j++){
            record* rec = node->values[j];
            count = 0;
            while(rec){
                count++;
                rec = rec->next;
            }            
            fwrite(&count, sizeof(int), 1, fp);
            rec = node->values[j];
            while(rec){
                fwrite(&(rec->pos), sizeof(unsigned int), 1, fp);
                rec = rec->next;
            }    
        }
        
        fwrite(&newline, sizeof(char),1, fp);
        //fwrite(endval, sizeof(char),strlen(endval), fp);    
        //fwrite(endnode, sizeof(char),strlen(endnode), fp);
        return;
    }

    for(int i = 0; i <= node->size; i++){
        //fwrite(startchild, sizeof(char),strlen(startchild), fp);
        commit_btree(fp, node->child[i], level+1);
        //fwrite(endchild, sizeof(char),strlen(endchild), fp);
    }
    //fwrite(endnode, sizeof(char),strlen(endnode), fp);
}

void commit_index(Table* table, Column* column){
    char filename[MAX_SIZE_NAME] = "";
    FILE* fp = NULL;

    switch(column->index->type){
        case CLUSTER_BTREE:
        {
            sprintf(filename, "%s-cluster-btree", column->name);
            fp = open_db_file(current_db->name, table->name, filename, "wb");
            if (fp != NULL) {
                commit_btree(fp, column->index->idxdata.treeidx.root, 0); 
                fflush(fp);           
            }
        }
        break;    
        case UNCLUSTER_BTREE:
        {
            sprintf(filename, "%s-uncluster-btree", column->name);
            fp = open_db_file(current_db->name, table->name, filename, "wb");
            if (fp != NULL) {
                commit_btree(fp, column->index->idxdata.treeidx.root, 0);  
                fflush(fp);          
            }
        }
        break;
        case UNCLUSTER_SORT:
            sprintf(filename, "%s-uncluster-sort", column->name);
            fp = open_db_file(current_db->name, table->name, filename, "wb");
            if (fp != NULL) {
                fwrite(column->index->idxdata.sortidx.copy_data, sizeof(int), column->col_size, fp);
                fwrite(column->index->idxdata.sortidx.pv->positions, sizeof(unsigned int), column->col_size, fp);
                fflush(fp);
            }
            break;
        default:
            break;
    }
    if(fp)fclose(fp);
}

void read_btree(FILE* fp, Column* column, Node* parent, int pos){  
    char c;
    //char data[32];
    Node* node = NULL;
    int nduplicates = 0;
    int level = 0, size = 0, type = 0;
    
    if (parent && (parent->type == LEAF || parent->type == RLEAF)) return;
    //read node tag    
    //fread(data, sizeof(char), strlen(startnode), fp);
    //read node size and type
    fread(&level, sizeof(int), 1, fp);
    fread(&size, sizeof(int), 1, fp);
    fread(&type, sizeof(int), 1, fp);
    fread(&c, sizeof(char), 1, fp);

    //create node and associate the parent
    node = btree_create_node(type);
    node->size = size;

    //set the child parent relationship
    if (parent) parent->child[pos] = node;

    //set the root node
    if (!parent) {
        column->index->idxdata.treeidx.root = node;
    }

    //read keys
    //data[0] = '\n';
    //fread(data, sizeof(char), strlen(startkeys), fp);
    fread(node->keys, sizeof(int), node->size, fp);
    fread(&c, sizeof(char), 1, fp);
    //data[0] = '\n';
    //fread(data, sizeof(char), strlen(endkeys), fp);
    //end reading keys

    if (node->type == LEAF || node->type == RLEAF) {
        //read position ids in leaf node

        //data[0] = '\n';
        //fread(data, sizeof(char), strlen(startval), fp);        
        for(int j = 0; j < node->size; j++){
            nduplicates = 0;
            fread(&nduplicates, sizeof(int), 1, fp);

            record* currec = NULL;
            record* prevrec = NULL;

            for(int k = 0; k < nduplicates; k++){
                currec = malloc(sizeof(record));
                currec->next = NULL;
                fread(&(currec->pos), sizeof(unsigned int), 1, fp);
                if (k == 0) node->values[j] = currec;
                if (prevrec) prevrec->next = currec;
                prevrec = currec;
            }
        }
        fread(&c, sizeof(char), 1, fp);
        //data[0] = '\n';
        //fread(data, sizeof(char), strlen(endval), fp);
        //data[0] = '\n';
        //fread(data, sizeof(char), strlen(endnode), fp);
        return;
    }

    //loop through child nodes
    for(int i = 0; i <= node->size; i++){
        //data[0] = '\n';
        //fread(data, sizeof(char), strlen(startchild), fp);
        read_btree(fp, column, node, i);
        if ( i != 0) {
            node->child[i]->left = node->child[i-1];
            node->child[i-1]->right = node->child[i];
        }
        //data[0] = '\n';
        //fread(data, sizeof(char), strlen(endchild), fp);
    }
    //end node tag
    //data[0] = '\n';
    //fread(data, sizeof(char), strlen(endnode), fp);
    return;
}

void read_index(Table* table, Column* column){
    char filename[MAX_SIZE_NAME] = "";
    FILE* fp = NULL;
    /*
    struct timeval st, et;
    gettimeofday(&st,NULL);
    */
   switch(column->index->type){
        case CLUSTER_BTREE:
        {
            sprintf(filename, "%s-cluster-btree", column->name);
            fp = open_db_file(current_db->name, table->name, filename, "rb");
            if (fp != NULL) 
                read_btree(fp, column, NULL, 0);            
        }
        break;    
        case UNCLUSTER_BTREE:
        {
            sprintf(filename, "%s-uncluster-btree", column->name);
            fp = open_db_file(current_db->name, table->name, filename, "rb");
            if (fp != NULL) 
                read_btree(fp, column, NULL, 0);            
        }
        break;
        case UNCLUSTER_SORT:
            sprintf(filename, "%s-uncluster-sort", column->name);
            fp = open_db_file(current_db->name, table->name, filename, "rb");
            if (fp != NULL) {
                column->index->idxdata.sortidx.pv = malloc(sizeof(PositionVector));
                column->index->idxdata.sortidx.pv->num_tuples = column->col_size;
                column->index->idxdata.sortidx.copy_data = (int*)calloc(column->col_size, sizeof(int));
                column->index->idxdata.sortidx.pv->positions = calloc(column->col_size, sizeof(unsigned int));
                fread(column->index->idxdata.sortidx.copy_data, sizeof(int), column->col_size, fp);
                fread(column->index->idxdata.sortidx.pv->positions, sizeof(unsigned int), column->col_size, fp);
            }
            break;
        default:
            break;
    }
    if(fp)fclose(fp);
    /*
    gettimeofday(&et,NULL);
    long elapsed = ((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec);
    cs165_log(stdout, "--Time taken for btree normal insert run %ld usec\n", elapsed);
    */
}