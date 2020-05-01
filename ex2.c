#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_BUFFER_SIZE 100
#define MAX_COMMAND_NUM 100
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
void get_command_args(char *command, int *command_status_ptr, char *args[MAX_BUFFER_SIZE]) {

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
void remove_last_arg(char *args[MAX_BUFFER_SIZE], int last_position) {
    args[last_position] = NULL;
}

/*
 *
 */
pid_t execute_background(char **args, int last_arg_position) {
    pid_t pid;
    int stat;
    int ret_code = 0;
    remove_last_arg(args, last_arg_position);
    if((pid = fork()) == 0) {
//        for(int i=0 ; i < MAX_BUFFER_SIZE ; i++) {
//            if(args[i] == NULL){
//                break;
//            }
//            printf("%s\n", args[i]);
//        }

        //print pid
        pid_t real_pid = getpid();
        printf("%d\n", real_pid);
        fflush(stdout);

        ret_code = execvp(args[0],args);
        sleep(10);
        if(ret_code == -1) {
            fprintf(stderr, ERROR_MSG);
        }
        exit(0);
    }
    else {
        return pid;
    }
}

/*
 *
 */
pid_t execute_foreground(char **args) {
    pid_t pid;
    int stat;
    int ret_code = 0;
    if((pid = fork()) == 0) {
//        for(int i=0 ; i < MAX_BUFFER_SIZE ; i++) {
//            if(args[i] == NULL){
//                break;
//            }
//            printf("%s\n", args[i]);
//        }

        //print pid
        pid_t real_pid = getpid();
        printf("%d\n", real_pid);
        fflush(stdout);

        ret_code = execvp(args[0],args);

        if(ret_code == -1) {
            fprintf(stderr, ERROR_MSG);
        }
        exit(0);
    }
    else {
        wait(&stat);
        return pid;
    }
}

/*
 *
 */
int get_last_parameter_position(char *args[MAX_BUFFER_SIZE]) {
    int last_arg_position = -1;
    for(int i=0 ; i < MAX_BUFFER_SIZE - 1; i++) {
        if(args[i + 1] == NULL){
            last_arg_position = i;
            break;
        }
    }
    if(last_arg_position == -1) {
        last_arg_position = MAX_BUFFER_SIZE - 1;
    }
    return last_arg_position;
}

char *get_status(const char *pid) {
    int status;
    pid_t received_pid = atoi(pid);
    pid_t return_pid = waitpid(received_pid, &status, WNOHANG); /* WNOHANG def'd in wait.h */

    // child is still running
    if (return_pid == 0) {
        return "RUNNING";
    }

    // child exited or error
    else {
        return "DONE";
    }
}

void add_to_history(pid_t pid, char *line, char ***history, int command_num) {
    char **log_line = malloc(sizeof(pid_t) + 2 * sizeof(char*));
    log_line[0] = malloc(sizeof(char*));
    sprintf(log_line[0],"%d", pid);
    log_line[1] = line;
    history[command_num] = log_line;
}

pid_t execute_history(char ***history, char *line, int command_num) {
    pid_t pid;
    int stat;
    if((pid = fork()) == 0) {

        pid_t real_pid = getpid();
        printf("%d\n", real_pid);
        add_to_history(real_pid, line, history, command_num);
        for(int i = 0 ; i < MAX_COMMAND_NUM ; i++) {
            if(history[i] == NULL) {
                break;
            }
            char **log_line = history[i];
            log_line[1][strlen(log_line[1]) - 1] = 0;
            char *status;
            if(i == command_num) {
                status = "RUNNING";
            }
            else {
                status = get_status(log_line[0]);
            }
            printf("%s %s %s\n", log_line[0], log_line[1], status);
//            printf("%s ", log_line[1]);
//            if(i == command_num) {
//                printf("RUNNING\n");
//            }
//            else {
//                printf("%s\n", get_status(log_line[0]));
//            }
            fflush(stdout);
        }
        history[command_num - 1][2] = "DONE";
        exit(0);
    }
    else {
        wait(&stat);
        return pid;
    }
}

/*
 *
 */
pid_t command_execute(char **args, char ***history, char *line, int command_num) {
    int last_arg_position = get_last_parameter_position(args);

    if(strcmp(args[0], HISTORY_COMMAND) == 0) {
        return execute_history(history, line, command_num);
    }

    //if command needs to run in the background
    else if(*args[last_arg_position] == BACKGROUND) {
        return execute_background(args, last_arg_position);
        //printf("in background");
    }
    //otherwise command needs to run in the foreground
    else {
        return execute_foreground(args);
        //printf("in foreground");
    }
}

/*
 *
 */
void init_history(char ***history) {
    for(int i=0 ; i < MAX_COMMAND_NUM ; i++) {
        history[i] = NULL;
    }
}

/*
 *
 */
void init_buffer(char **args) {
    for(int i=0 ; i < MAX_BUFFER_SIZE ; i++) {
        args[i] = NULL;
    }
}

void free_buffer(char *line, char *args[MAX_BUFFER_SIZE]) {
    free(line);
    for(int i=0 ; i < MAX_BUFFER_SIZE ; i++) {
        free(args[i]);
    }
    free(args);
}

/*
 *
 */
void command_loop(void) {
    char *line;
    int command_status = 1;
    int command_num = 0;
    int *command_status_ptr = &command_status;
    char ***history =  malloc(MAX_COMMAND_NUM * sizeof(char**));
    init_history(history);

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

        char **args = malloc(MAX_BUFFER_SIZE * sizeof(char*));
        init_buffer(args);

        get_command_args(line, command_status_ptr, args);
        if (*command_status_ptr == 0) {
            fprintf(stderr, "???");
            continue;
        }
//        for(int i=0 ; i < MAX_BUFFER_SIZE ; i++) {
//            if(args[i] == NULL){
//                break;
//            }
//            printf("%s\n", args[i]);
//        }
        //executing the command

        pid_t pid = command_execute(args, history, line, command_num);

        //adding command to history (if command is "history", it will be added as part of it's own run)
        if(args[0] != HISTORY_COMMAND) {
            add_to_history(pid, line, history, command_num);
        }
        //free resources
        //free_buffer(line, args);
        command_num++;
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
