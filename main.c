#include<errno.h> //preprocessor macro used for error indication
#include<fcntl.h> //for open()
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<stdint.h>
#include<unistd.h> //for open()

typedef struct
{
    char* buffer;
    size_t buffer_length; //size_t to store a size in bytes
    ssize_t input_length; 
    /* signed size_t,to store a size in bytes or negative value(-1) error
    , not included in standard library and isn't portable*/
} InputBuffer;

/*instead of using exceptions (C doesn't support exception handling)
like how we would use in say Python, 
we simply use enum result codes
*/
//meta commands: Non-SQL commands like .exit (all start with a dot)
//we handle these commands in a different function

//The two enums indicate success or failure
typedef enum{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR,
    PREPARE_STRING_TOO_LONG,
    PREPARE_NEGATIVE_ID
} PrepareResult;

//just two values for now
typedef enum{
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef enum{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
}ExecuteResult;


#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE+1];
    char email[COLUMN_EMAIL_SIZE+1];
} Row;
// (Struct*)0 is a null pointer but does not get dereferenced by
// -> cuz here sizeof simply returns the amount of bytes allocated by definition of the data type ( I hope this is right)
#define size_of_attribute(Struct,Attribute) sizeof(((Struct*)0)->Attribute)
//int32_t = fixed size of 32 bits unlike int (which can have any size>=16 bits)
const uint32_t ID_SIZE = size_of_attribute(Row,id); // 4 bytes
const uint32_t USERNAME_SIZE = size_of_attribute(Row,username); // 33 bytes
const uint32_t EMAIL_SIZE = size_of_attribute(Row,email); //256 bytes

const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET+ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE; //293 bytes

typedef struct{
    StatementType type;
    Row row_to_insert; //only used by insert statement
} Statement;

//Row to memory
void serialize_row(Row* source, void* destination){
    memcpy(destination+ID_OFFSET, &(source->id),ID_SIZE);
    memcpy(destination+USERNAME_OFFSET,&(source->username),USERNAME_SIZE);
    memcpy(destination+EMAIL_OFFSET,&(source->email),EMAIL_SIZE);
}
//Memory to row
void deserialize_row(void* source, Row* destination){
    memcpy(&(destination->id),source+ID_OFFSET,ID_SIZE);
    memcpy(&(destination->username),source+USERNAME_OFFSET,USERNAME_SIZE);
    memcpy(&(destination->email),source+EMAIL_OFFSET,EMAIL_SIZE);
}

const uint32_t PAGE_SIZE = 4096; //4Kbs same as a page used in most virtual memory systems in most comp. architectures.
#define TABLE_MAX_PAGES 100
// const uint32_t ROWS_PER_PAGE = PAGE_SIZE/ROW_SIZE; //4096/291 = 14
// const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

//Leaf nodes and internal nodes have different layouts.
//To keep track of node type
//Each node corresponds to one page
typedef enum{
    NODE_INTERNAL,
    NODE_LEAF
}NodeType;

//Every node is going to take up exactly one page

//Common Node header format
/* Metadata needed to be stored in a header at the beginning of
    the page: what type of node it is, whether root node or not,
    pointer to its parent (3 total)*/
//sizeof(uint8_t) indicates 1 byte of memory
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
//offset 0 indicates node type field starts at the beginning
const uint32_t NODE_TYPE_OFFSET = 0; 
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET+IS_ROOT_SIZE;
const uint8_t COMMON_NODE_HEADER_SIZE = 
        NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE; //or = PARENT_POINTER_OFFSET + PARENT_POINTER_SIZE

/*Leaf Node Format*/
/* In addition to the common header fields,
LEAF nodes need to store how many "cells" they contain.
A cell = key:value pair*/
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = 
        COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE;

/*Leaf Node Body Layout*/
/*The body of a leaf node = array of cells*/
/* Each key is followed by a value*/
/*value = serialized row*/
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_KEY_OFFSET = 0;
const uint32_t LEAF_NODE_VALUE_SIZE = ROW_SIZE;
const uint32_t LEAF_NODE_VALUE_OFFSET = 
        LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE;
const uint32_t LEAF_NODE_CELL_SIZE = LEAF_NODE_KEY_SIZE+LEAF_NODE_VALUE_SIZE;
const uint32_t LEAF_NODE_SPACE_FOR_CELLS = PAGE_SIZE - LEAF_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_MAX_CELLS = 
        LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE;

/*Accessing Leaf Node Fields*/
/* These methods return a pointer in question,
so they can be used as both a getter and a setter*/
/*The code to access keys, values and metadata all involve
pointer arithmetic using the constants we just defined*/
uint32_t* leaf_node_num_cells(void* node){
    return node+LEAF_NODE_NUM_CELLS_OFFSET;
}

void* leaf_node_cell(void* node, uint32_t cell_num){
    return node+LEAF_NODE_HEADER_SIZE + cell_num*LEAF_NODE_CELL_SIZE;
}

uint32_t* leaf_node_key(void* node, uint32_t cell_num){
    return leaf_node_cell(node, cell_num);
}

void* leaf_node_value(void* node, uint32_t cell_num){
    return leaf_node_cell(node,cell_num)+LEAF_NODE_KEY_SIZE;
}

void initialize_leaf_node(void* node){
    uint32_t* num_cells_offset = leaf_node_num_cells(node);
    *num_cells_offset = 0;
}



//The Pager struct: accesses file and page cache
typedef struct{
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void* pages[TABLE_MAX_PAGES];
}Pager;

typedef struct{
    Pager* pager;
    uint32_t root_page_num; //to keep track of the btree
}Table;

typedef struct{
    Table* table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table; //indicates a position one past the last element.
}Cursor;
void* get_page(Pager* pager, uint32_t page_num);

Cursor* table_start(Table* table){
    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    // cursor->row_num = 0;
    // cursor->end_of_table = (table->num_rows==0); //if table empty: true
    cursor->page_num = table->root_page_num;
    cursor->cell_num = 0;

    void* root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->end_of_table = (num_cells == 0);
    return cursor;
}

Cursor* table_end(Table* table){
    Cursor* cursor = malloc(sizeof(Cursor));
    cursor->table = table;
    // cursor->row_num = table->num_rows;
    cursor->page_num = table->root_page_num;

    void* root_node = get_page(table->pager, table->root_page_num);
    uint32_t num_cells = *leaf_node_num_cells(root_node);
    cursor->cell_num = num_cells;
    cursor->end_of_table = true;
    return cursor;
}

void* cursor_value(Cursor* cursor){
    uint32_t page_num = cursor->page_num;
    void* page = get_page(cursor->table->pager,page_num);
    // uint32_t row_offset = row_num % ROWS_PER_PAGE;
    // uint32_t byte_offset = row_offset * ROW_SIZE;
    // return page + byte_offset;
    return leaf_node_value(page,cursor->cell_num);
}

void cursor_advance(Cursor* cursor){
    // cursor->row_num += 1;
    // if(cursor->row_num >= cursor->table->num_rows){
    uint32_t page_num = cursor->page_num;
    void* node = get_page(cursor->table->pager, page_num);
    cursor->cell_num += 1;
    if(cursor->cell_num >= (*leaf_node_num_cells(node))){       
        cursor->end_of_table = true;
    }
}
void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value){
    void* node = get_page(cursor->table->pager,cursor->page_num);

    uint32_t num_cells = *leaf_node_num_cells(node);
    if(num_cells>=LEAF_NODE_MAX_CELLS){
        //Node full
        printf("Need to implement splitting of a leaf node.\n");
        exit(EXIT_FAILURE);
    }

    if(cursor->cell_num < num_cells){
        //Insert row at pos cell_num
        //shift all the following cells to the right
        for(uint32_t i = num_cells; i>cursor->cell_num; i--){
            memcpy(leaf_node_cell(node,i),leaf_node_cell(node,i-1),LEAF_NODE_CELL_SIZE);
        }
    }
    //now insert into cell_num
    *(leaf_node_num_cells(node)) += 1;
    *(leaf_node_key(node,cursor->cell_num)) = key;
    serialize_row(value,leaf_node_value(node,cursor->cell_num));
}

void print_prompt(){ printf("db > ");}

void print_row(Row* row){
    printf("(%d, %s, %s)\n",row->id, row->username, row->email);
}

void print_constants(){
    printf("ROW_SIZE: %d\n",ROW_SIZE);
    printf("COMMON_NODE_HEADER_SIZE: %d\n", COMMON_NODE_HEADER_SIZE);
    printf("LEAF_NODE_HEADER_SIZE: %d\n",LEAF_NODE_HEADER_SIZE);
    printf("LEAF_NODE_CELL_SIZE: %d\n",LEAF_NODE_CELL_SIZE);
    printf("LEAF_NODE_SPACE_FOR_CELLS: %d\n", LEAF_NODE_SPACE_FOR_CELLS);
    printf("LEAF_NODE_MAX_CELLS: %d\n",LEAF_NODE_MAX_CELLS);
}

void print_leaf_node(void* node){
    uint32_t num_cells = *leaf_node_num_cells(node);
    
}

ExecuteResult execute_insert(Statement* statement,Table* table){
    // if(table->num_rows >= TABLE_MAX_ROWS){
    //     return EXECUTE_TABLE_FULL;
    // }
    void* node = get_page(table->pager,table->root_page_num);
    if(*(leaf_node_num_cells(node)) >=LEAF_NODE_MAX_CELLS){
        return EXECUTE_TABLE_FULL;
    }

    Row* row_to_insert = &(statement->row_to_insert);
    Cursor* cursor = table_end(table);
    // serialize_row(row_to_insert, cursor_value(cursor));
    // table->num_rows += 1;
    leaf_node_insert(cursor,row_to_insert->id,row_to_insert);

    free(cursor);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* statement, Table* table){
    Row row;
    Cursor* cursor = table_start(table);
    // for(uint32_t i = 0; i<table->num_rows; i++){
        // deserialize_row(row_slot(table,i),&row);
    while(!(cursor->end_of_table)){
        deserialize_row(cursor_value(cursor),&row);
        print_row(&row);
        cursor_advance(cursor);
    }

    free(cursor);
    return EXECUTE_SUCCESS;
}
void db_close(Table* table);
MetaCommandResult do_meta_command(InputBuffer* input_buffer,Table* table){
    if (!strcmp(input_buffer->buffer,".exit")){
        // printf("freed\n");
        // free(table);
        db_close(table);
        exit(EXIT_SUCCESS);
    }else{
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement){
    statement->type = STATEMENT_INSERT;
    char* keyword = strtok(input_buffer->buffer, " ");
    char* id_string = strtok(NULL, " ");
    char* username = strtok(NULL, " ");
    char* email = strtok(NULL, " ");

    if(id_string==NULL || username==NULL || email==NULL){
        return PREPARE_SYNTAX_ERROR;
    }
    int id = atoi(id_string);
    //how to make sure id_string is a number?
    if(id<0){
        return PREPARE_NEGATIVE_ID;
    }
    if(strlen(username)>COLUMN_USERNAME_SIZE) return PREPARE_STRING_TOO_LONG;
    if(strlen(email)>COLUMN_EMAIL_SIZE) return PREPARE_STRING_TOO_LONG;
    
    statement->row_to_insert.id = id;
    strcpy(statement->row_to_insert.username,username);
    strcpy(statement->row_to_insert.email,email);
    return PREPARE_SUCCESS;
}

//Our "SQL compiler". Now our compiler only understands two words
PrepareResult prepare_statement(InputBuffer* input_buffer,Statement* statement){
    if (!strncmp(input_buffer->buffer,"insert",6)){
        return prepare_insert(input_buffer, statement);

        //scanf when inputting more than it should buffer overflows so we'll first 
        //break down the string into the three keywords and then check their length
        //using strtok
        // statement->type = STATEMENT_INSERT;
        // int args_assigned = sscanf(
        //     input_buffer->buffer,"insert %d %s %s",&(statement->row_to_insert.id),
        //      statement->row_to_insert.username, statement->row_to_insert.email
        // );
        // if(args_assigned<3){
        //     return PREPARE_SYNTAX_ERROR;
        // }
        // return PREPARE_SUCCESS;
    }
    if (!strcmp(input_buffer->buffer,"select")){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult execute_statement(Statement* statement,Table* table){
    if(statement->type == STATEMENT_INSERT){
        return execute_insert(statement,table);
    }else if(statement->type == STATEMENT_SELECT){
        return execute_select(statement,table);
    }
}
InputBuffer* new_input_buffer(){
    InputBuffer* input_buffer = malloc(sizeof(InputBuffer));
    //initiliazing 
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;

    return input_buffer;
}
Pager* pager_open(const char* filename){
    int fd = open(filename, 
                O_RDWR| //Read/write mode
                    O_CREAT, //create file if it does not exit
                S_IWUSR | //user write permission
                    S_IRUSR ); //user read permission
    if(fd == -1){
        printf("Unable to open file\n");
        exit(EXIT_FAILURE);
    }
    //use off_t for file sizes
    off_t file_length = lseek(fd,0,SEEK_END);

    Pager* pager = malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    
    if(file_length % PAGE_SIZE !=0){
        printf("Db file is not a whole no. of pages. Corrupt file.\n");
        exit(EXIT_FAILURE);
    }

    for(uint32_t i = 0;i< TABLE_MAX_PAGES; i++){
        pager->pages[i] = NULL;
    }
    return pager;
}
// Table* new_table(){
Table* db_open(const char* filename){
    Pager* pager = pager_open(filename);
    // uint32_t num_rows = pager->file_length / ROW_SIZE;
    Table* table = malloc(sizeof(Table));
    // table->num_rows = num_rows; //if new file table->num_rows = 0
    table->pager = pager;
    table->root_page_num = 0;

    if(pager->num_pages==0){
        //New file, intiliaze page 0 as leaf node
        void* root_node = get_page(pager,0);
        initialize_leaf_node(root_node);
    } 
    return table;
}

void* get_page(Pager* pager, uint32_t page_num){
    if(page_num > TABLE_MAX_PAGES){
        printf("Tried to fetch page number out of bounds. %d > %d \n",
        page_num,TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if(pager->pages[page_num] == NULL){
        //Cache miss, Allocate memory and load from file
        void* page = malloc(PAGE_SIZE);
        uint32_t num_pages = pager->file_length/PAGE_SIZE;
        if(pager->file_length%PAGE_SIZE!=0){
            num_pages += 1;
        }
        // if the requested page_num is within the bounds of the file.
        if(page_num <= num_pages){
            /*set the file offset to the beginning of the desired page (page_num*PAGE_SIZE)*/
            lseek(pager->file_descriptor, page_num*PAGE_SIZE, SEEK_SET); 
            //reads PAGE_SIZE no. of bytes from the file descriptor to the buffer (page)
            ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);        
            if(bytes_read == -1){
                printf("Error reading file: %d\n", errno);
                exit(EXIT_FAILURE);
            }
        }

        pager->pages[page_num] = page;

        if(page_num>=pager->num_pages){
            pager->num_pages += 1;
        }
    }
    return pager->pages[page_num];

}

//size is also needed cuz of the possibility of partial pages
void pager_flush(Pager* pager, uint32_t page_num){
    if(pager->pages[page_num]==NULL){
        printf("Tried to flush null page\n");
        exit(EXIT_FAILURE);
    }

    off_t offset= lseek(pager->file_descriptor, page_num*PAGE_SIZE, SEEK_SET);
    if(offset==-1){
        printf("Error seeking: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_written = 
        write(pager->file_descriptor, pager->pages[page_num],PAGE_SIZE);
    if(bytes_written == -1){
        printf("Error writing: %d\n",errno);
        exit(EXIT_FAILURE);
    }
}


//flushes the page cache to disk, closes the db file, frees Pager and Table data structures
void db_close(Table* table){
    Pager* pager = table->pager;
    // uint32_t num_full_pages = table->num_rows/ROWS_PER_PAGE;

    for(uint32_t i = 0; i<pager->num_pages; i++){
        if(pager->pages[i] == NULL){
            continue;
        }
        pager_flush(pager, i);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    //Free partial page
    // uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
    // if(num_additional_rows > 0){
    //     //partial page exists
    //     uint32_t page_num = num_full_pages;
    //     if(pager->pages[page_num]!=NULL){
    //         pager_flush(pager,page_num,num_additional_rows*ROW_SIZE);
    //         free(pager->pages[page_num]);
    //         pager->pages[page_num] = NULL;
    //     }
    // }

    int result = close(pager->file_descriptor);
    if(result == -1){
        printf("Error closing db file.\n");
        exit(EXIT_FAILURE);
    }
    //Making sure all pages are freed from memory?
    for(uint32_t i =0; i<TABLE_MAX_PAGES; i++){
        void* page = pager->pages[i];
        if(page){
            free(page);
            pager->pages[i] = NULL;
        }
    }
    free(pager);
    free(table);
}
// void free_table(Table* table){
//     for(uint32_t i = 0; i<TABLE_MAX_PAGES; i++){
//         free(table->pages[i]);
//     }
//     free(table);
// }

//ssize_t getline(char** lineptr, size_t *n, FILE *stream)
/*lineptr: pointer, if it is NULL, it will be mallocatted by getline
so even if the command fails, you have to free it
here we don't mallocate input_buffer->buffer so it is done by line ptr */
void read_input(InputBuffer* input_buffer){
    ssize_t bytes_read = 
        getline(&(input_buffer->buffer), &(input_buffer->buffer_length),stdin);
    
    if(bytes_read <=0){
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }

    input_buffer->input_length = bytes_read -1;
    input_buffer->buffer[bytes_read-1]= 0;
}

void close_input_buffer(InputBuffer* input_buffer){
    free(input_buffer->buffer);
    free(input_buffer);
}


int main(int argc, char* argv[]){
    // printf("%d\n",ROW_SIZE);
    // printf("%d\n",PAGE_SIZE);
    // printf("%d",ROWS_PER_PAGE);
    InputBuffer* input_buffer =new_input_buffer();
    Statement statement;
    // Table* table = new_table();
    if(argc<2){
        printf("Must supply a database filename.\n");
        exit(EXIT_FAILURE);
    }
    char* filename = argv[1];
    Table* table = db_open(filename);
    print_constants();
    while(true){
        print_prompt();
        read_input(input_buffer);
        //handling meta commands
        if(input_buffer->buffer[0] == '.'){
            switch(do_meta_command(input_buffer,table)){
                case(META_COMMAND_SUCCESS): continue;
                case(META_COMMAND_UNRECOGNIZED_COMMAND):
                    printf("Unrecognized command '%s'\n",input_buffer->buffer);
                    continue;
            }
        }
        //if not a meta command ==> SQL command
        Statement statement;
        switch(prepare_statement(input_buffer,&statement)){
            case(PREPARE_SUCCESS): 
                break;
            case(PREPARE_UNRECOGNIZED_STATEMENT):
                printf("Unrecognized keyword at start of '%s'.\n",
                    input_buffer->buffer);
                //Read input again
                continue;
            case(PREPARE_NEGATIVE_ID):
                printf("ID must be positive. \n");
                continue;
            case(PREPARE_STRING_TOO_LONG):
                printf("String is too long.\n");
                continue;
        } 

        switch (execute_statement(&statement,table)){
            case (EXECUTE_SUCCESS):
                printf("Executed.\n");
                break;
            case (EXECUTE_TABLE_FULL):
                printf("Error: Table full.\n");
                break;
        }
    }
    // free_table(table);
    // printf("freed\n");
    return 0;
}