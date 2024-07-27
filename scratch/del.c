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


uint32_t* leaf_node_num_cells(void* leaf_node){
    return leaf_node + LEAF_NODE_NUM_CELLS_OFFSET;
}

void* leaf_node_cell(void* leaf_node, uint32_t cell_num){
    return leaf_node + LEAF_NODE_HEADER_SIZE + cell_num * LEAF_NODE_NUM_CELLS_SIZE;
}

uint32_t* leaf_node_key(void* leaf_node, uint32_t cell_num){
    return leaf_node_cell(leaf_node,cell_num);
}

void* leaf_node_value(void* leaf_node, uint32_t cell_num){
    return leaf_node_cell(leaf_node, cell_num) + LEAF_NODE_KEY_SIZE;
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
void* get_page(Table* table,int page_num){
    uint32_t table_full_pages = table->num_rows/ROWS_PER_PAGE;
    uint32_t num_additional_rows = table->num_rows%ROWS_PER_PAGE;
    int fd = table->pager->file_descriptor;
    void* page = malloc(PAGE_SIZE);
    if(!page){
        printf("Error allocating page\n");
        exit(EXIT_FAILURE);
    }
    // table->pager->pages[page_num] = malloc(PAGE_SIZE);
    lseek(fd,page_num*PAGE_SIZE,SEEK_SET);
    read(fd,page, PAGE_SIZE);
    printf("Returned page.\n");
    return page;
}

void* row_slot(Table* table,int row_num){
    int page_num = row_num/ROWS_PER_PAGE;
    void* page = table->pager->pages[page_num];
    int rows_at_last_page = row_num%ROWS_PER_PAGE;
    // printf("Accessing page:%d rows_at_last_page:%d for row_num %d\n",page,rows_at_last_page,row_num);
    if(page == NULL){
        if(row_num<table->num_rows){
            table->pager->pages[page_num] = get_page(table, page_num);
        }
        else if(table->num_rows%ROWS_PER_PAGE>0){
            table->pager->pages[page_num] = get_page(table, page_num);
        }
        else{
            table->pager->pages[page_num] = malloc(PAGE_SIZE);
        }
        page = table->pager->pages[page_num];
        printf("Allocated %p to %p\n",page,page+PAGE_SIZE);
    }
    return page+(rows_at_last_page*ROW_SIZE);
}

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

EXECUTE_RESULT execute_statement(Statement* statement,Table* table){
    if(statement->type==STATEMENT_INSERT){
        Row row;
        int args_assigned = sscanf(statement->input_buffer,
        "insert %d %s %s",
        &(row.id),row.username,row.email);


        if(args_assigned < 3){
            return INSERT_SYNTAX_ERROR;
        }
        void* dest = row_slot(table,table->num_rows);
        // return INSERT_SYNTAX_ERROR;
        serialize_row(&row,dest);
        table->num_rows += 1;
        print_row(row);
        return INSERT_SUCCESS;
    }
    else if(statement->type==STATEMENT_SELECT){
        printf("Table rows: %d\n",table->num_rows);
        for(int i =0; i<table->num_rows;i++){
            Row current_row;
            void* source = row_slot(table,i);
            deserialize_row(source,&current_row);
            print_row(current_row);
        }
    }
    return SELECT_SUCCESS;
}



int do_meta_command(char* input_buffer){
    if(!strcmp(input_buffer,".exit")){
        return 1;
    }
    return 0;
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
    cursor->page_num = table->num_rows/ROWS_PER_PAGE + (int)(table->num_rows%ROWS_PER_PAGE!=0);
    cursor->end_of_table = true;
    return cursor;
}

void cursor_advance(Cursor* cursor){
    
}

Table* create_table(const char* filename){
    Table* table = malloc(sizeof(Table));
    table->pager = pager_open(filename);
    uint32_t file_length = table->pager->file_length;
    table->num_rows = file_length/ROW_SIZE;
    printf("Num_rows: %d\n",table->num_rows);
    return table;
}

void free_pager(Table* table){
    Pager* pager = table->pager;
    
    printf("TABLE MAX PAGES: %d", TABLE_MAX_PAGES);
    int num_full_pages = table->num_rows/ROWS_PER_PAGE;
    int num_additional_rows = table->num_rows%ROWS_PER_PAGE;
    for (uint32_t i = 0; i<num_full_pages; i++) {

        if(pager->pages[i]==NULL) continue;

        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }
    if(num_additional_rows>0){
        if(pager->pages[num_full_pages]!=NULL){
            free(pager->pages[num_full_pages]);
            pager->pages[num_full_pages]=NULL;
        }
    }
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
int main(int argc, char* argv[]){
    if(argc!=2){
        printf("Provide filename\n");
        exit(1);
    }
    const char* filename = argv[1];
    // char buffer[BUFSIZE];
    Table* table = create_table(filename);
    printf("ROW_SIZE:%d\n",ROW_SIZE);
    Statement statement;
    // printf("%d",ROWS_PER_PAGE);
    while(true){
        printf("db > ");
        char* d = fgets(statement.input_buffer,BUFSIZE,stdin);
        statement.input_buffer[strlen(statement.input_buffer)-1] = '\0';
        if(statement.input_buffer[0] == '.'){
            switch(do_meta_command(statement.input_buffer)){    
                case (1): 
                    db_close(table);
                    exit(EXIT_SUCCESS);
                    break;
                default: 
                    printf("Unrecognized command.\n");
                    break;
            }
        }else{
            prepare_statement(&statement);
            execute_statement(&statement,table);
        }

    }
    return 0;
}