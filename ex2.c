#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/*
 * Reading the input line from stdin
 */
char *get_line(int *command_status_ptr) {
    //defining the buffer
    ssize_t buffer_size = 0;
    char *line = NULL;

    //getting the line
    if(getline(&line, &buffer_size, stdin) == -1) {
        if(!feof(stdin)) {
            *command_status_ptr = 0; //didnt get to EOF, therefore read wasn't successful
        }
    }
    return line;
}

int token(char *begin, char *end){

}
/*
 * Parsing the received line into a list of arguments
 */
#define BUFFER_SIZE 100
void get_command_args(char *command, int *command_status_ptr, char *args[BUFFER_SIZE]) {

    int position = 0;

    char *arg;
    char *begin;

    for(;;) {
        //skipping spaces
        while (isspace(*command)) {
            *command++;
        }

        //if we reached the end of the command
        if(*command == '\0') {
            break;
        }
        if(isalnum(*command) || *command == '-') {
            begin = command;
            //todo change to everthing that is not space or /0
            while(isalnum(*command) || *command == '-') {
                *command++;
            }
            arg = malloc((command-begin) * sizeof(char));
            strncpy(arg, begin, command - begin);
            args[position] = arg;
            position++;
            arg = NULL;
        }
        //if we have quotes in the command
        if(*command =='"') {
            int quote = *command++;
            begin = command;

            //going over the command until we reached the closing quote
            while (*command && *command != '"') {
                *command++;
                //if we reached the end of the command, without getting a closing quote, update status to error
                if (*command == '\0') {
                    *command_status_ptr = 0;
                }
            }
            *command++;
            if (*command_status_ptr == 0) {
                break;
            }
            arg = malloc((command-begin) * sizeof(char));
            strncpy(arg, begin, command - begin);
            args[position] = arg;
            position++;
            arg = NULL;
        }
    }
}
void command_loop(void) {
    char *line;
    int command_status = 1;
    int *command_status_ptr = &command_status;
    int status;

    do {
        printf("> ");

        //reading the command
        line = get_line(command_status_ptr);
        if (*command_status_ptr == 0) {
            fprintf(stderr, "???");
            continue;
        }
        printf("%s\n", line);

        int buffer_size = BUFFER_SIZE;
        char *args[buffer_size];
        for(int i=0 ; i<buffer_size ; i++) {
            args[i] = NULL;
        }

        get_command_args(line, command_status_ptr, args);
        if (*command_status_ptr == 0) {
            fprintf(stderr, "???");
            continue;
        }
        for(int i=0 ; i<BUFFER_SIZE ; i++) {
                printf("%c\n", *args[i]);
        }
        //executing the command
        //status = command_execute(args);
        free(line);
        free(args);
    } while(status);
}

int main() {

    //running the command loop
    command_loop();

    return 0;
}
