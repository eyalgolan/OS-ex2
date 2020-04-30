#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 100
#define BACKGROUND '&'
#define STARTER "> "
#define ERROR_MSG "Error in system call"
#define HISTORY_COMMAND "history"

/*
 * Reading the input line from stdin
 */
char *get_line(int *command_status_ptr) {
    //defining the buffer
    size_t buffer_size = 0;
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
        if(*command == BACKGROUND) {
            arg = malloc(sizeof(char) + 1);
            strncpy(arg, command, 1);
            arg[strlen(arg)] = '\0';
            args[position] = arg;
            position++;
            *command++;
            arg = NULL;
        }
        if(isalnum(*command) || *command == '-') {
            begin = command;
            //todo change to everthing that is not space or /0
            while(isalnum(*command) || *command == '-') {
                *command++;
            }

            arg = malloc((command-begin + 1) * sizeof(char));
            strncpy(arg, begin, command - begin);
            arg[strlen(arg)] = '\0';
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
            arg = malloc((command-begin + 1) * sizeof(char));
            strncpy(arg, begin, command - begin);
            arg[strlen(arg)] = '\0';
            args[position] = arg;
            position++;
            arg = NULL;
        }
    }
}

/*
 *
 */
void remove_last_arg(char *args[BUFFER_SIZE], int last_position) {
    args[last_position] = NULL;
}

/*
 *
 */
void execute_background(char **args, int last_arg_position) {
    pid_t pid;
    int stat;
    int ret_code = 0;
    remove_last_arg(args, last_arg_position);
    if((pid = fork()) == 0) {
//        for(int i=0 ; i < BUFFER_SIZE ; i++) {
//            if(args[i] == NULL){
//                break;
//            }
//            printf("%s\n", args[i]);
//        }
        ret_code = execvp(args[0],args);
        if(ret_code == -1) {
            fprintf(stderr, ERROR_MSG);
        }
        exit(EXIT_SUCCESS);
    }
    else {
        //wait(&stat);
    }
}

/*
 *
 */
void execute_foreground(char **args) {
    pid_t pid;
    int stat;
    int ret_code = 0;
    if((pid = fork()) == 0) {
//        for(int i=0 ; i < BUFFER_SIZE ; i++) {
//            if(args[i] == NULL){
//                break;
//            }
//            printf("%s\n", args[i]);
//        }
        ret_code = execvp(args[0],args);
        pid_t real_pid = getpid();

        if(ret_code == -1) {
            fprintf(stderr, ERROR_MSG);
        }
    }
    else {
        wait(&stat);
    }
}

/*
 *
 */
int get_last_parameter_position(char *args[BUFFER_SIZE]) {
    int last_arg_position = -1;
    for(int i=0 ; i < BUFFER_SIZE - 1; i++) {
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

void execute_history(char **history) {

}
/*
 *
 */
char **command_execute(char **args, char **history) {
    int last_arg_position = get_last_parameter_position(args);

    if(args[0] == HISTORY_COMMAND) {
        execute_history(history);
    }

    //if command needs to run in the background
    if(*args[last_arg_position] == BACKGROUND) {
        execute_background(args, last_arg_position);
        //printf("in background");
    }
    //otherwise command needs to run in the foreground
    else {
        execute_foreground(args);
        //printf("in foreground");
    }
}

/*
 *
 */
void init_array(char **args) {
    for(int i=0 ; i<BUFFER_SIZE ; i++) {
        args[i] = NULL;
    }
}

void free_buffer(char *line, char *args[BUFFER_SIZE]) {
    free(line);
    for(int i=0 ; i < BUFFER_SIZE ; i++) {
        free(args[i]);
    }
    free(args);
}

void add_to_history(char *pid_status, char *line, char **history) {

}
/*
 *
 */
void command_loop(void) {
    char *line;
    int command_status = 1;
    int *command_status_ptr = &command_status;
    char **history =  malloc(BUFFER_SIZE * sizeof(char*));
    init_array(history);

    int status = 1;
    do {
        printf(STARTER);

        //reading the command
        line = get_line(command_status_ptr);
        if (*command_status_ptr == 0) {
            fprintf(stderr, "???");
            continue;
        }
//        printf("%s\n", line);

        char **args = malloc(BUFFER_SIZE * sizeof(char*));
        init_array(args);

        get_command_args(line, command_status_ptr, args);
        if (*command_status_ptr == 0) {
            fprintf(stderr, "???");
            continue;
        }
//        for(int i=0 ; i < BUFFER_SIZE ; i++) {
//            if(args[i] == NULL){
//                break;
//            }
//            printf("%s\n", args[i]);
//        }
        //executing the command
        char **pid_status = command_execute(args, history);
        add_to_history(pid_status, line, history);
        //free resources
        //free_buffer(line, args);

    } while(status);
}

/*
 *
 */
int main() {
    //running the command loop
    command_loop();

    return 0;
}
