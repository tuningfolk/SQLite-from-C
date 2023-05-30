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


int main(int argc, char* argv[]){
    InputBuffer* read = new_input_buffer();
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
    }
    return 0;
}