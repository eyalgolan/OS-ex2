#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024
char *get_line(void) {
    //defining and allocating the buffer
    int buffer_size = BUFFER_SIZE;
    char *buffer = malloc(sizeof(char) * buffer_size);

    //defining the current char we handle
    int curr_position = 0;
    int curr_char;

    if(!buffer) {
        fprintf(stderr, "???");
        exit(EXIT_FAILURE);
    }

    //going over the input and adding it to the buffer
    curr_char = getchar(); //reading the first char
    while(curr_char != EOF && curr_char != '\n') {
        buffer[curr_position] = curr_char;
        curr_char = getchar(); //reading the next char
        curr_position++;
    }
    buffer[curr_position] = '\0'; //when we get to the end, we replace it with a null character
    return buffer;
}

char **get_command_args(char *command) {

}
void command_loop(void) {
    char *line;
    char **command_args;
    int status;

    do {
        printf("> ");

        //reading the command
        line = get_line();
        command_args = get_command_args(line);

        //executing the command
        status = command_execute(args);
        free(command);
        free(command_args);
    } while(status);
}

int main() {

    //running the command loop
    command_loop();

    return 0;
}
