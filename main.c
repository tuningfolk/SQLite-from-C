#include<stdio.h>
#include<stdint.h>
#include<stdbool.h>
#include<stddef.h>
#include<stdlib.h>
#include<string.h>

typedef struct InputBuffer{
    char* buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

typedef enum MetaCommandResult {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum StatementType{
    STATEMENT_INSERT,
    STATEMENT_SELECT
}StatementType;

typedef enum PrepareResult{
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT
}PrepareResult;

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 32
typedef struct Row{
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
}Row;
typedef struct Statement{
    StatementType type;
    Row row_to_insert; //only used by insert statement
}Statement;

InputBuffer* new_input_buffer(){
    InputBuffer* new = malloc(sizeof(InputBuffer));
    new->buffer = NULL;
    new->buffer_length = 0;
    new->input_length = 0;
    return new;
}

//reads input from user
void read_input(InputBuffer* input){
    ssize_t bytes_read = getline(&(input->buffer),&(input->buffer_length), stdin);

    if(bytes_read <= 0){
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }

    input->input_length = bytes_read-1;
    input->buffer[bytes_read-1] = '\0';
}
//frees memory
void close_input_buffer(InputBuffer* input){
    free(input->buffer);
    free(input);
}

void print_prompt() { printf("sqlite> "); }

MetaCommandResult do_meta_command(InputBuffer* input){
    if (strcmp(input->buffer, ".exit") == 0){
        close_input_buffer(input);
        exit(EXIT_SUCCESS);
        // return META_COMMAND_SUCCESS; // Not needed ig
    }else{
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

PrepareResult prepare_statement(InputBuffer* input, Statement* statement){
    if(strncmp(input->buffer, "insert", 6)==0){
        statement->type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }else if(strncmp(input->buffer, "select", 6)==0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult execute_statement(Statement* statement){
    switch (statement->type)
    {
    case  STATEMENT_INSERT:
        // printf("This is where you would do an insert.\n");
        // break;
        return execute_insert();
    case STATEMENT_SELECT:
        // printf("This is where you do a select.\n");
        // break;
        return execute_select();
    default:
        break;
    }
}

#define size_of_attribute(Struct, Attribute) sizeof( ((Struct*)0)->Attribute )

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0; //I know why you might need this, but I can't tell how you would use this
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = EMAIL_OFFSET + EMAIL_SIZE; //or ID_SIZE + USERNAME_SIZE + EMAIL_SIZE

/* 4 kilobytes because it's the same as a page used in the virtual systems of most computer architectures,
this means one page in our database corresponds to one page in the operating system, the os will move pages in and out
of memory as whole units instead of breaking them up*/
/* 100 pages = arbitrary*/

const uint32_t PAGE_SIZE = 4096;
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE/ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct Table{
    uint32_t num_rows;
    void* pages[TABLE_MAX_PAGES];
}Table;

Table* create_table(){
    Table* table= malloc(sizeof(Table));
    table->num_rows = 0;
    for(int i=0; i<TABLE_MAX_PAGES; i++){
        table->pages[i] = NULL;
    }
    return table;
}

void free_table(Table* table){
    for(int i=0; table->pages[i]; i++){ //an interesting way to put the limiting case
        free(table->pages[i]);
    }
    free(table);
}


typedef enum ExecuteResult{
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
}ExecuteResult;

void* row_slot(Table* table, uint32_t row_num){
    uint32_t page_num = row_num/ROWS_PER_PAGE;
    uint32_t row_offset = row_num%ROWS_PER_PAGE;
    void* page = table->pages[page_num];
    if(page == NULL){
        page = table->pages[page_num] = malloc(PAGE_SIZE);
    }
    uint32_t byte_offset = page + row_offset*ROW_SIZE;
    return page+byte_offset;
}

void


int main(int argc, char* argv[]){
    InputBuffer* read = new_input_buffer();
    // Statement* statement = malloc(sizeof(Statement)); //why won't this work?
    Statement statement;
    while(true){
        print_prompt();
        read_input(read);
        if(read->buffer[0] == '.'){
            switch (do_meta_command(read))
            {
            case META_COMMAND_SUCCESS:
                continue;
                // break;
            case META_COMMAND_UNRECOGNIZED_COMMAND:
                printf("Unrecognized command '%s' .\n", read->buffer);
                continue;
            // default:
            //     break;
            }
        }
        switch (prepare_statement(read, &statement)){
            case PREPARE_SUCCESS:
                break;

            case PREPARE_UNRECOGNIZED_STATEMENT:
                printf("Unrecognized keyword at start of '%s'.\n", read->buffer);
                continue;
        }

        
        execute_statement(&statement);

    }
    return 0;
}