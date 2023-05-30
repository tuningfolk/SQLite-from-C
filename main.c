#include<stdio.h>
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

typedef struct Statement{
    StatementType type;
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

void execute_statement(Statement* statement){
    switch (statement->type)
    {
    case  STATEMENT_INSERT:
        printf("This is where you would do an insert.\n");
        break;
    case STATEMENT_SELECT:
        printf("This is where you do a select.\n");
        break;
    default:
        break;
    }
}



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

        prepare_statement(read, &statement);
        execute_statement(&statement);

    }
    return 0;
}