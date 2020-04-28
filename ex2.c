#include <stdio.h>
#include <stdlib.h>

/*
 * Reading the input line from stdin
 */
char *get_line(void) {
    //defining the buffer
    ssize_t buffer_size = 0;
    char *line = NULL;

    //getting the line
    if(getline(&line, &buffer_size, stdin) == -1) {
        if(feof(stdin)) {
            exit(EXIT_SUCCESS); //received EOF, therefore read was successful
        }
        else {
            //todo what needs to happen here?
            fprintf(stderr, "???");
            exit(EXIT_FAILURE);
        }
    }

    return line;
}

/*
 * Parsing the recieved line into a list of arguments
 */
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
