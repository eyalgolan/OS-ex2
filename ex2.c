#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

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

/*
 * Parsing the received line into a list of arguments
 */
#define BUFFER_SIZE 100
void get_command_args(char *command, int *command_status_ptr, char *args[BUFFER_SIZE]) {

    int position = 0; //position in argument array
    char *arg; //pointer to current arg
    char *begin; //arg's starting position in the command

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

void execute_background(char *args[BUFFER_SIZE]) {

}

void execute_foreground(char *args[BUFFER_SIZE]) {

}

int get_last_parameter_position(char *args[BUFFER_SIZE]) {
    int last_arg_position = -1;
    for(int i=0 ; i<BUFFER_SIZE - 1; i++) {
        if(args[i + 1] == NULL){
            last_arg_position = i;
            break;
        }
    }
    if(last_arg_position == -1) {
        last_arg_position = BUFFER_SIZE - 1;
    }
    return last_arg_position;
}
void command_execute(char *args[BUFFER_SIZE]) {
    int last_arg_position = get_last_parameter_position(args);
    execvp(args[0],args);

    //if command needs to run in the background
    if(args[last_arg_position] == "&") {
        execute_background(args);
    }
    //otherwise command needs to run in the foreground
    else {
        execute_foreground(args);
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
            if(args[i] == NULL){
                break;
            }
            printf("%s\n", args[i]);
        }
        //executing the command
        command_execute(args);

        //free resources
        free(line);
        for(int i=0 ; i<BUFFER_SIZE ; i++) {
            free(args[i]);
        }
        //free(args);
    } while(status);
}

int main() {

    //running the command loop
    command_loop();

    return 0;
}
