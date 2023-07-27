#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<stdint.h>

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
    PREPARE_SYNTAX_ERROR
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
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
} Row;
// (Struct*)0 is a null pointer but does not get dereferenced by
// -> cuz here sizeof simply returns the amount of bytes allocated by definition of the data type ( I hope this is right)
#define size_of_attribute(Struct,Attribute) sizeof(((Struct*)0)->Attribute)
//int32_t = fixed size of 32 bits unlike int (which can have any size>=16 bits)
const uint32_t ID_SIZE = size_of_attribute(Row,id); // 4 bytes
const uint32_t USERNAME_SIZE = size_of_attribute(Row,username); // 32 bytes
const uint32_t EMAIL_SIZE = size_of_attribute(Row,email); //255 bytes

const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET+ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

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
const uint32_t ROWS_PER_PAGE = PAGE_SIZE/ROW_SIZE; //4096/291 = 14
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct{
    uint32_t num_rows;
    void* pages[TABLE_MAX_PAGES];
}Table;

void* row_slot(Table* table,uint32_t row_num){
    uint32_t page_num = row_num/ROWS_PER_PAGE;
    void* page = table->pages[page_num];
    if(page==NULL){
        //Allocate memory only when we need the page
        page = table->pages[page_num] = malloc(PAGE_SIZE);
    }
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset;
}

void print_prompt(){ printf("db > ");}

void print_row(Row* row){
    printf("(%d, %s, %s)\n",row->id, row->username, row->email);
}

ExecuteResult execute_insert(Statement* statement,Table* table){
    if(table->num_rows >= TABLE_MAX_ROWS){
        return EXECUTE_TABLE_FULL;
    }
    Row* row_to_insert = &(statement->row_to_insert);
    // printf("%s",row_to_insert->email);
    serialize_row(row_to_insert,row_slot(table,table->num_rows));
    table->num_rows += 1;
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* statement, Table* table){
    Row row;
    for(uint32_t i = 0; i<table->num_rows; i++){
        deserialize_row(row_slot(table,i),&row);
        print_row(&row);
    }
    return EXECUTE_SUCCESS;
}
MetaCommandResult do_meta_command(InputBuffer* input_buffer,Table* table){
    if (!strcmp(input_buffer->buffer,".exit")){
        // printf("freed\n");
        free(table);
        exit(EXIT_SUCCESS);
    }else{
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

//Our "SQL compiler". Now our compiler only understands two words
PrepareResult prepare_statement(InputBuffer* input_buffer,Statement* statement){
    if (!strncmp(input_buffer->buffer,"insert",6)){
        statement->type = STATEMENT_INSERT;
        int args_assigned = sscanf(
            input_buffer->buffer,"insert %d %s %s",&(statement->row_to_insert.id),
             statement->row_to_insert.username, statement->row_to_insert.email
        );
        if(args_assigned<3){
            return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
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
Table* new_table(){
    Table* table = malloc(sizeof(Table));
    table->num_rows = 0;
    for(uint32_t i = 0; i<TABLE_MAX_PAGES; i++){
        table->pages[i] = NULL;
    }
    return table;
}
void free_table(Table* table){
    for(uint32_t i = 0; i<TABLE_MAX_PAGES; i++){
        free(table->pages[i]);
    }
    free(table);
}

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
    Table* table = new_table();
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
            case(PREPARE_SUCCESS): break;
            case(PREPARE_UNRECOGNIZED_STATEMENT):
                printf("Unrecognized keyword at start of '%s'.\n",
                    input_buffer->buffer);
                //Read input again
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
    free_table(table);
    // printf("freed\n");
    return 0;
}