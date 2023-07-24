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
    while(true){
        print_prompt();
        read_input(input_buffer);

        if (!strcmp(input_buffer->buffer,".exit")){
            close_input_buffer(input_buffer);
            exit(EXIT_SUCCESS);
        }
        printf("Unrecognized statement '%s'.\n", input_buffer->buffer);
    }
    return 0;
}
