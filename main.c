#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>

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
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

//just two values for now
typedef enum{
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef struct{
    StatementType type;
} Statement;

MetaCommandResult do_meta_command(InputBuffer* input_buffer){
    if (!strcmp(input_buffer->buffer,".exit")){
        exit(EXIT_SUCCESS);
    }else{
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

//Our "SQL compiler". Now our compiler only understands two words
PrepareResult prepare_statement(InputBuffer* input_buffer,Statement* statement){
    if (!strncmp(input_buffer->buffer,"insert",6)){
        statement->type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }
    if (!strcmp(input_buffer->buffer,"select")){
        statement->type =STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement* statement){
    if(statement->type == STATEMENT_SELECT){
        printf("This is where you would do a select\n");
    }else if(statement->type == STATEMENT_INSERT){
        printf("This is where you would do an insert\n");
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

void print_prompt(){ printf("db > ");}

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
    InputBuffer* input_buffer =new_input_buffer();
    Statement statement;
    while(true){
        print_prompt();
        read_input(input_buffer);
        //handling meta commands
        if(input_buffer->buffer[0] == '.'){
            switch(do_meta_command(input_buffer)){
                case(META_COMMAND_SUCCESS): continue;
                case(META_COMMAND_UNRECOGNIZED_COMMAND):
                    printf("Unrecognized command '%s'\n",input_buffer->buffer);
                    continue;
            }
        }
        //if not a meta command ==> SQL command
        switch(prepare_statement(input_buffer,&statement)){
            case(PREPARE_SUCCESS): break;
            case(PREPARE_UNRECOGNIZED_STATEMENT):
                printf("Unrecognized keyword at start of '%s'.\n",
                    input_buffer->buffer);
                //Read input again
                continue;
        }

        execute_statement(&statement);
        printf("Executed.\n");
    }
    return 0;
}
