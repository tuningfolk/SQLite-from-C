#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
const uint32_t ID_SIZE = sizeof(uint32_t);
#define USERNAME_SIZE 33
#define EMAIL_SIZE  256
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET+USERNAME_SIZE;
#define BUFSIZE 350
#define TABLE_MAX_PAGES 100
#define PAGE_SIZE 4096
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE/ROW_SIZE;

typedef enum{
    STATEMENT_INSERT,
    STATEMENT_SELECT
}StatementType;

typedef enum{
    INSERT_SUCCESS,
    INSERT_SYNTAX_ERROR,
    EXECUTE_DUPLICATE_KEY,
    EXECUTE_TABLE_FULL,
    SELECT_SUCCESS
} EXECUTE_RESULT;

typedef enum{ NODE_INTERNAL, NODE_LEAF} NodeType;

//Common Node Header Layout
const uint8_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint8_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint8_t PARENT_POINTER_SIZE = sizeof(uint32_t);

const uint8_t NODE_TYPE_OFFSET = 0;
const uint8_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint8_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET+IS_ROOT_SIZE;
const uint8_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE
                                    +IS_ROOT_SIZE
                                    +PARENT_POINTER_SIZE;


//Leaf Node Header Layout
const uint8_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint8_t LEAF_NODE_HEADER_SIZE =
    COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

const uint8_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;

//Leaf Node Body Layout
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET = LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE =
    LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS =
    PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = 
    LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;
     
//FOR REFERENCE,NOT USED ANYWHERE
const uint32_t WASTED_SPACE = LEAF_NODE_SPACE_FOR_CELLS % LEAF_NODE_CELL_SIZE;

//Internal Node Header Layout
const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t); 

const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET = INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;

const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE+
                                            INTERNAL_NODE_NUM_KEYS_SIZE+
                                            INTERNAL_NODE_RIGHT_CHILD_SIZE;

// Internal Node Body Layout
const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CELL_SIZE =
                            INTERNAL_NODE_KEY_SIZE+INTERNAL_NODE_CHILD_SIZE;

		

NodeType get_node_type(void* node){
    /*Casting as uint8_t to ensure its stored as a single byte*/
    uint8_t value =  *(NodeType*)node;
    return (NodeType)value;
}

bool is_node_root(void* node){
    uint8_t value = *(uint8_t*)(node+IS_ROOT_OFFSET);
    return (bool)value;
}
void set_node_root(void* node, bool is_root){
    uint8_t value = is_root;
    *(uint8_t*)(node+IS_ROOT_OFFSET) = value;
}
void set_node_type(void* node, NodeType type){
    /*Casting as uint8_t to ensure its stored as a single byte*/
    *(uint8_t*)node = (uint8_t)type;
}




uint32_t* leaf_node_num_cells(void* leaf_node){
    return leaf_node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void* leaf_node_cell(void* leaf_node, uint32_t cell_num){
    return leaf_node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(void* leaf_node, uint32_t cell_num){
    return leaf_node_cell(leaf_node,cell_num);
}

void* leaf_node_value(void* leaf_node, uint32_t cell_num){
    return leaf_node_cell(leaf_node, cell_num) + LEAF_NODE_KEY_SIZE;
}

void initialize_leaf_node(void* node){
    set_node_type(node, NODE_LEAF);
    set_node_root(node,false);
    *leaf_node_num_cells(node) = 0;
}
uint32_t* internal_node_num_keys(void* internal_node){
	return internal_node+INTERNAL_NODE_NUM_KEYS_OFFSET;
}
void initialize_internal_node(void* node){
	set_node_type(node, NODE_INTERNAL);
    set_node_root(node,false);
	*internal_node_num_keys(node) = 0;
}	

uint32_t* internal_node_right_child(void* node){
    return node + INTERNAL_NODE_RIGHT_CHILD_OFFSET;
}

uint32_t* internal_node_cell(void* node, uint32_t cell_num){
    return node+INTERNAL_NODE_HEADER_SIZE+cell_num*INTERNAL_NODE_CELL_SIZE;
}

uint32_t* internal_node_child(void* node, uint32_t child_num){
    uint32_t num_keys = *internal_node_num_keys(node);
    if(child_num>num_keys){
        printf("Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
        exit(EXIT_FAILURE);
    }else if(child_num == num_keys){
        return internal_node_right_child(node);
    }else{
        return internal_node_cell(node, child_num);
    }
}

uint32_t* internal_node_key(void* node, uint32_t key_num){
    return internal_node_cell(node,key_num) + INTERNAL_NODE_CHILD_SIZE;
}

uint32_t get_node_max_key(void* node){
    switch (get_node_type(node)) {
        case NODE_INTERNAL:
            return *internal_node_key(node, *internal_node_num_keys(node)-1);
        case NODE_LEAF:
            return *leaf_node_key(node, *leaf_node_num_cells(node)-1);
    }
}


typedef struct{
    StatementType type;
    char input_buffer[BUFSIZE];
}Statement;

typedef struct{
    uint32_t id;
    char username[USERNAME_SIZE];
    char email[EMAIL_SIZE];
}Row;
typedef struct {
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void* pages[TABLE_MAX_PAGES];
}Pager;
typedef struct{
    uint32_t root_page_num;
    Pager* pager;
}Table;

typedef struct{
    Table* table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table;
}Cursor;

int prepare_statement(Statement* statement){
    if(strncmp(statement->input_buffer,"insert",6)==0){
        statement->type = STATEMENT_INSERT;
        return 1;
    }else if(strcmp(statement->input_buffer,"select")==0){
        statement->type = STATEMENT_SELECT;
        return 1;
    }
    return 0;
}

int get_unused_page_num(Pager* pager){
    return pager->num_pages;
}

void* get_page(Table* table,int page_num){
    // uint32_t table_full_pages = table->num_rows/ROWS_PER_PAGE;
    // uint32_t num_additional_rows = table->num_rows%ROWS_PER_PAGE;
    if(page_num >= TABLE_MAX_PAGES){
        printf("Tried to access page that is out-of-bounds.\n");
        exit(EXIT_FAILURE);
    }
    if(table->pager->pages[page_num]!=NULL){
        return table->pager->pages[page_num]; 
    }
    int fd = table->pager->file_descriptor;
    if(page_num>= table->pager->num_pages){
        table->pager->num_pages += 1;
    }
    void* page = malloc(PAGE_SIZE);
    if(!page){
        printf("Error allocating page\n");
        exit(EXIT_FAILURE);
    }
    table->pager->pages[page_num] = page;
    // table->pager->pages[page_num] = malloc(PAGE_SIZE);
    lseek(fd,page_num*PAGE_SIZE,SEEK_SET);
    read(fd,page, PAGE_SIZE);
    printf("Returned page.\n");
    return page;
}

// void* row_slot(Table* table,int row_num){
//     int page_num = row_num/ROWS_PER_PAGE;
//     void* page = table->pager->pages[page_num];
//     int rows_at_last_page = row_num%ROWS_PER_PAGE;
//     // printf("Accessing page:%d rows_at_last_page:%d for row_num %d\n",page,rows_at_last_page,row_num);
//     if(page == NULL){
//         if(row_num<table->num_rows){
//             table->pager->pages[page_num] = get_page(table, page_num);
//         }
//         else if(table->num_rows%ROWS_PER_PAGE>0){
//             table->pager->pages[page_num] = get_page(table, page_num);
//         }
//         else{
//             table->pager->pages[page_num] = malloc(PAGE_SIZE);
//         }
//         page = table->pager->pages[page_num];
//         printf("Allocated %p to %p\n",page,page+PAGE_SIZE);
//     }
//     return page+(rows_at_last_page*ROW_SIZE);
// }

void print_row(Row row){
    printf("Row %d %s %s\n",row.id,row.username,row.email);
}



void serialize_row(Row* source,void* dest){
    // void* dest = row_slot(table,table->num_rows);
    // table->num_rows += 1;
    // memset(dest, 0, ROW_SIZE);
    memcpy(dest+ID_OFFSET,&(source->id),ID_SIZE);
    memcpy(dest+USERNAME_OFFSET,&(source->username),USERNAME_SIZE);
    memcpy(dest+EMAIL_OFFSET,&(source->email),EMAIL_SIZE);
    // printf("%d %s %s\n",*(int*)dest,(char *)(dest+USERNAME_OFFSET),(char *)(dest+EMAIL_OFFSET));
}

void deserialize_row(void* source,Row* dest){
    // void* src = row_slot(table, row_num);
    // printf("Accessing row at %p", source);
    memcpy(&(dest->id),source,ID_SIZE);
    memcpy(&(dest->username),(source+USERNAME_OFFSET),USERNAME_SIZE);
    memcpy(&(dest->email),(source+EMAIL_OFFSET),EMAIL_SIZE);
}
Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key){
    void* node = get_page(table, page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    
    //Initializing Cursor
    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = page_num;
    
    // Binary search
    uint32_t min_index = 0;
    uint32_t one_past_max_index = num_cells;
    while(min_index<one_past_max_index){
        uint32_t middle_index = (min_index+one_past_max_index) / 2;
        uint32_t middle_key = *leaf_node_key(node, middle_index);
        if(middle_key == key){
            cursor->cell_num = middle_index;
            return cursor;
        }else if(middle_key > key){
            one_past_max_index = middle_index;
        }else{
            min_index = middle_index + 1;
        }
    }
    cursor->cell_num = min_index;
    return cursor;
}

Cursor* cursor_start(Table* table){
    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->page_num = table->root_page_num;
    cursor->cell_num = 0;
    void* root_node=  get_page(table,table->root_page_num);
    uint8_t root_num_cells = *leaf_node_num_cells(root_node);
    cursor->end_of_table = (root_num_cells==0);
    return cursor;
}

Cursor* cursor_end(Table* table){
    Cursor* cursor = malloc(sizeof(Cursor));
    
    cursor->table = table;
    cursor->page_num = table->root_page_num;
    void* root_node = get_page(table, table->root_page_num);
    cursor->cell_num = *leaf_node_num_cells(root_node);
    cursor->end_of_table = true;;
    return cursor;
}

Cursor* cursor_find(Table* table, uint32_t key){
    /*
    Return the position of the given key
    if the key is not present, 
        return the position where it should be inserted.
    */
    uint32_t root_page_num = table->root_page_num;
    void* root_node = get_page(table,root_page_num);
    if(get_node_type(root_node)==NODE_LEAF){
        return leaf_node_find(table, root_page_num,key);
    }else{
        printf("Need to implement searching an internal node.\n");
        exit(EXIT_FAILURE); //
    }
}

void create_new_root(Table* table, uint32_t left_child_page_num, uint32_t right_child_page_num){
	uint32_t new_root_page_num = get_unused_page_num(table->pager);
	void* new_root = get_page(table, new_root_page_num);
	initialize_internal_node(new_root);

}

void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value){
    void* old_node = get_page(cursor->table,cursor->page_num);
    uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
    void* new_node = get_page(cursor->table,new_page_num);
    initialize_leaf_node(new_node);
    uint32_t left_num_cells = (LEAF_NODE_MAX_CELLS+1)/2;
    uint32_t right_num_cells = LEAF_NODE_MAX_CELLS + 1 - left_num_cells;
    Cursor* new_cell = leaf_node_find(cursor->table,cursor->page_num , key);
    for(int i = 0; i<=LEAF_NODE_MAX_CELLS; i++){
        void *source, *dest;
        /* find dest*/
         if(i<left_num_cells){
            dest = leaf_node_cell(old_node,i);
        }else{
            dest = leaf_node_cell(new_node,i%(left_num_cells));
        }
        /* find source */
        if(i < new_cell->cell_num){
            source = leaf_node_cell(old_node, i);
        }else if(i > new_cell->cell_num){
            source = leaf_node_cell(old_node,  i-1);
        }else{
            /* If cell to be inserted */	
            source = malloc(LEAF_NODE_CELL_SIZE);
            memcpy(source,&key, sizeof(uint32_t));
            /*make space for source*/
            memcpy(source+LEAF_NODE_VALUE_OFFSET, value, ROW_SIZE);
        }
        
        if(dest!=source) memcpy(dest,source,LEAF_NODE_CELL_SIZE); /*If source and dest dont overlap(ie not same)*/
    }
 
}

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value){
    void* node = get_page(cursor->table,cursor->page_num);
    // printf("are we inserting at the root page? %s\n",
                        // (cursor->page_num==0)?"yes":"no");
    uint32_t num_cells = *leaf_node_num_cells(node);
    if(num_cells>=LEAF_NODE_MAX_CELLS){
        //Node full
        printf("need to implement splitting a leaf node.\n");
        leaf_node_split_and_insert(node, key, value);
        return;
    }
    printf("Cursor cell_num: %d, num_cells: %d\n", cursor->cell_num, num_cells);
    if(cursor->cell_num<num_cells){
        for(uint32_t i = num_cells; i>cursor->cell_num;i--){
            // memset(leaf_node_cell(node,i),0,LEAF_NODE_CELL_SIZE);
            void* source = leaf_node_value(node,i-1);
            Row temp_row;
            deserialize_row(source,&temp_row);
            printf("Shifting ..");
            print_row(temp_row);
            memcpy(leaf_node_cell(node, i), 
            leaf_node_cell(node,i-1), 
            LEAF_NODE_CELL_SIZE);
        }
    }
    printf("About to insert: ");
    print_row(*value);
    // printf("Page location: %p\n",node);
    // printf("Current no. of cells %d\n",
            // *leaf_node_num_cells(node));
    *leaf_node_num_cells(node) += 1;
    *leaf_node_key(node, cursor->cell_num)=key;
    serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

EXECUTE_RESULT execute_statement(Statement* statement,Table* table){
    if(statement->type==STATEMENT_INSERT){
        Row row_to_insert;
        int args_assigned = sscanf(statement->input_buffer,
        "insert %d %s %s",
        &(row_to_insert.id),row_to_insert.username,row_to_insert.email);

        if(args_assigned < 3){
            return INSERT_SYNTAX_ERROR;
        }
        // void* dest = row_slot(table,table->num_rows);
        void* node = get_page(table,table->root_page_num);
        uint32_t num_cells = *leaf_node_num_cells(node);
        if(num_cells>=LEAF_NODE_MAX_CELLS){
            printf("need to implement splitting a leaf node.\n");
            return EXECUTE_TABLE_FULL;
        }
        //Getting an index to maintain sort, so changing cursor position
        // Cursor* cursor = cursor_end(table);

        uint32_t key_to_insert = row_to_insert.id;
        Cursor* cursor = cursor_find(table, key_to_insert);
        printf("Cell position: %d\n",cursor->cell_num);
        // printf("Number of cells: %d\n",cursor->cell_num);
        if(cursor->cell_num<num_cells){
            uint32_t key_at_index = *leaf_node_key(node,cursor->cell_num);
            
            if(key_at_index == key_to_insert){
                return EXECUTE_DUPLICATE_KEY;
            }
        }
        leaf_node_insert(cursor,key_to_insert,&row_to_insert);
        free(cursor);
        // return INSERT_SYNTAX_ERROR;
        // serialize_row(&row_to_insert,dest); --> We'll add this in leaf_node_insert
        // table->num_rows += 1;
        // Cursor* cursor = cursor_end()l
        // leaf_node_insert(, uint32_t key, Row *value);
        // print_row(row_to_insert);
        return INSERT_SUCCESS;
    }
    // else if(statement->type==STATEMENT_SELECT){
    //     printf("Table rows: %d\n",table->num_rows);
    //     for(int i =0; i<table->num_rows;i++){
    //         Row current_row;
    //         void* source = row_slot(table,i);
    //         deserialize_row(source,&current_row);
    //         print_row(current_row);
    //     }
    // }
    return SELECT_SUCCESS;
}



Pager* pager_open(const char* filename){
    Pager* pager = malloc(sizeof(Pager));
    if(!pager){
        printf("Failed to allocate memory for pager.\n");
        exit(EXIT_FAILURE);
    }
    int fd = open(filename,
                    O_RDWR| //Read/Write mode
                        O_CREAT,  //Create file if it does not exit
                    S_IWUSR| //User write permission
                        S_IRUSR); //User read permission
    if(fd==-1){
        printf("Unable to open file");
        //need to free memory
        exit(EXIT_FAILURE);
    }

    off_t file_length = lseek(fd, 0, SEEK_END);
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    pager->num_pages=  file_length/PAGE_SIZE;
    if(file_length%PAGE_SIZE!=0){
        printf("Db is not a whole number of pages, Corrupt.\n");
        exit(EXIT_FAILURE);
    }
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }
    return pager;
}

void* cursor_value(Cursor* cursor){
    void* page =get_page(cursor->table,cursor->page_num);
    return leaf_node_value(page, cursor->cell_num);
}

void cursor_advance(Cursor* cursor){
    void* node = get_page(cursor->table, cursor->page_num);
    cursor->cell_num += 1;
    if(cursor->cell_num>=*leaf_node_num_cells(node)){
        cursor->end_of_table = true;
    }
}


Table* db_open(const char* filename){
    Table* table = malloc(sizeof(Table));
    table->pager = pager_open(filename);
    uint32_t file_length = table->pager->file_length;
    // table->num_rows = file_length/ROW_SIZE;
    table->root_page_num = 0;
    if(table->pager->num_pages==0){
        //New database file, Initialize page 0 as leaf node
        void* root_node = get_page(table,0);
        initialize_leaf_node(root_node);
    }
    // printf("Num_rows: %d\n",table->num_rows);
    return table;
}

void free_pager(Table* table){
    Pager* pager = table->pager;
    
    printf("TABLE MAX PAGES: %d", TABLE_MAX_PAGES);
    // int num_full_pages = table->num_rows/ROWS_PER_PAGE;
    // int num_additional_rows = table->num_rows%ROWS_PER_PAGE;
    for (uint32_t i = 0; i<pager->num_pages; i++) {

        if(pager->pages[i]==NULL) continue;

        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }
    // if(num_additional_rows>0){
    //     if(pager->pages[num_full_pages]!=NULL){
    //         free(pager->pages[num_full_pages]);
    //         pager->pages[num_full_pages]=NULL;
    //     }
    // }
    printf("\n");
    free(pager);
}

void free_table(Table* table){
    free_pager(table);

    free(table);
}
void page_flush(Table* table){
    Pager* pager = table->pager;
    // uint32_t table_full_pages = table->num_rows/ROWS_PER_PAGE;
    // uint32_t num_additional_rows = table->num_rows%ROWS_PER_PAGE;
    int fd = pager->file_descriptor;
    for(uint32_t i = 0; i<pager->num_pages; i++){
        if(pager->pages[i]==NULL) continue;
        off_t offset = lseek(fd,i*PAGE_SIZE , SEEK_SET);
        if(offset==-1){
            printf("Error seeking: %d\n",errno);
            exit(EXIT_FAILURE);
        }
        ssize_t bytes_written = write(fd,pager->pages[i],PAGE_SIZE );
        printf("Bytes flushed: %ld\n",bytes_written);
    }
    //Removing partial pages vv
    // if(num_additional_rows>0){
    //     //If last page null, it has not been updated, continue
    //     if(pager->pages[table_full_pages]!=NULL){
    //         //Cache miss, page not updated, continue
            
    //         off_t offset = lseek(fd,table_full_pages*PAGE_SIZE , SEEK_SET);
    //         if(offset==-1){
    //             printf("Error seeking: %d\n",errno);
    //             exit(EXIT_FAILURE);
    //         }
    //         ssize_t bytes_written = write(fd,pager->pages[table_full_pages],PAGE_SIZE );
    //         printf("Bytes flushed: %ld\n",bytes_written);
    //     }
    // }

}

void db_close(Table* table){
    page_flush(table);
    close(table->pager->file_descriptor);
    free_table(table);
}

void print_leaf_node(void* node){
    uint32_t num_cells = *leaf_node_num_cells(node);
    printf("Leaf size: %d\n",num_cells);
    for(uint32_t i = 0;i<num_cells; i++){
        printf(" - %d : %d\n",i,*leaf_node_key(node,i));
    }
}

void print_constants(){
    printf("ROW_SIZE: %d\n", ROW_SIZE);
    printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
    printf("LEAF_NODE_HEADER_SIZE: %d\n", LEAF_NODE_HEADER_SIZE);
    printf("LEAF_NODE_CELL_SIZE: %d\n", LEAF_NODE_CELL_SIZE);
    printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
    printf("LEAF_NODE_MAX_CELLS: %d\n", LEAF_NODE_MAX_CELLS);
}

int do_meta_command(char* input_buffer,Table* table){
    if(!strcmp(input_buffer,".exit")){
        db_close(table);
        exit(EXIT_SUCCESS);
        return 1;
    }else if(!strcmp(input_buffer, ".btree")){
        print_leaf_node(get_page(table, 0));
        return 1;
    }else if(!strcmp(input_buffer,".constants")){
        printf("Constants:\n");
        print_constants();
        return 1;
    }
    return 0;
}


int main(int argc, char* argv[]){
    if(argc!=2){
        printf("Provide filename\n");
        exit(1);
    }
    const char* filename = argv[1];
    // char buffer[BUFSIZE];
    Table* table = db_open(filename);
    printf("ROW_SIZE:%d\n",ROW_SIZE);
    Statement statement;
    // printf("%d",ROWS_PER_PAGE);
    while(true){
        table->root_page_num = 0;
        if(table->pager->num_pages==0){
            //New database file, Initialize page 0 as leaf node
            void* root_node = get_page(table,0);
            initialize_leaf_node(root_node);
        }
        printf("db > ");
        char* d = fgets(statement.input_buffer,BUFSIZE,stdin);
        statement.input_buffer[strlen(statement.input_buffer)-1] = '\0';
        if(statement.input_buffer[0] == '.'){
            switch(do_meta_command(statement.input_buffer,table)){    
                case (1): 
                    // exit(EXIT_SUCCESS);
                    break;
                default: 
                    printf("Unrecognized command.\n");
                    break;
            }
        }else{
            prepare_statement(&statement);
            switch (execute_statement(&statement,table)) {
                case(EXECUTE_TABLE_FULL):
                    printf("Table full.\n");
                    break;
                case (EXECUTE_DUPLICATE_KEY):
                    printf("Duplicate Key.\n");
                    break;
                case (INSERT_SYNTAX_ERROR):
                    printf("Syntax Error\n");
                    break;
                default:
                    break;
            };
        }

    }
    return 0;
}
