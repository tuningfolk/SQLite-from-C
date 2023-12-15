#include<stdbool.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>


typedef struct{
    char* buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer; 
InputBuffer* new_input_buffer(){
    InputBuffe
}

int main(int argc, char* argv){
    InputBuffer* input_buffer = new_input_buffer();
    while(true){
        print_prompt();
        read_input(input_buffer);

        if(strcmp(input_buffer->buffer, ".exit")==0){
            close_input_buffer(input_buffer);
            exit(EXIT_SUCCESS);
        }else{
            printf("Unrecognized command '%s'.\n", input_buffer->buffer);
        }
    }
}