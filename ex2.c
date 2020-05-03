#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>

#define MAX_BUFFER_SIZE 100
#define MAX_COMMAND_NUM 100
#define ERROR_CODE -999
#define BACKGROUND '&'
#define STARTER "> "
#define ERROR_MSG "Error in system call"
#define ARGS_NUM_ERROR "Error: Too many arguments"
#define DIRECTORY_NOT_FOUND_ERROR "ERROR: no such file or directory"
#define EXIT_COMMAND "exit"
#define CD_COMMAND "cd"
#define HISTORY_COMMAND "history"
#define JOBS_COMMAND "jobs"
#define RUNNING_STATUS "RUNNING"
#define DONE_STATUS "DONE"

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
        if(isalnum(*command) || *command == '-' || *command == '~' || *command == '.') {
            begin = command;
            //todo change to everthing that is not space or /0
            while(isalnum(*command) || *command == '-' || *command == '~' || *command == '.') {
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
        //sleep(10);
        if(ret_code == -1) {
            fprintf(stderr, ERROR_MSG);
            fflush(stderr);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
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
            fflush(stderr);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    else {
        wait(&stat);
        //sleep(1); //todo is this ok?
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

/*
 *
 */
char *get_status(const char *pid) {
    int status;
    pid_t received_pid = atoi(pid);
    pid_t return_pid = waitpid(received_pid, &status, WNOHANG); /* WNOHANG def'd in wait.h */

    // child is still running
    if (return_pid == 0) {
        //printf("running status");
        //fflush(stdout);
        return RUNNING_STATUS;
    }

    // child exited or error
    else {
        //printf("done status\n");
        //printf("%d\n", return_pid);
        //printf("%d\n", received_pid);
        //fflush(stdout);
        return DONE_STATUS;
    }
}

/*
 *
 */
void add_to_logger(pid_t pid, char *line, char ***logger, int command_num) {
    logger[command_num] = malloc(3*sizeof(char*));
    logger[command_num][0] = malloc(sizeof(char*));
    snprintf(logger[command_num][0],sizeof(char*),"%d", pid);
    logger[command_num][1] = malloc(sizeof(char*));
    strcpy(logger[command_num][1], line);
    int i = 0;
    while(logger[i] != NULL && i < MAX_COMMAND_NUM) {
        //printf("printing before adding %d %s %s %s\n", i, logger[i][0],logger[i][1], logger[i][2]);
        i++;
    }
    //printf("adding to log line %d\n", command_num);
    //logger[command_num] = log_line;
}

/*
 *
 */
pid_t execute_jobs(char ***jobs) {
    pid_t pid;
    int stat;
    int real_index = 0;

    if ((pid = fork()) == 0) {

        pid_t real_pid = getpid();
        printf("%d\n", real_pid);

        for (int i = 0 ; i < MAX_COMMAND_NUM ; i++) {
            if (jobs[i] == NULL) {
                break;
            }
            char **log_line = jobs[i];
            log_line[1][strlen(log_line[1]) - 1] = 0;
            char *status;
            status = get_status(log_line[0]);
            printf("%s %s %s\n", log_line[0], log_line[1], status);
            fflush(stdout);
            real_index++;
        }
        exit(EXIT_SUCCESS);
    }
    else {
        wait(&stat);
        return pid;
    }
}

/*
 *
 */
pid_t execute_history(char ***history, char *line, int command_num) {
    pid_t pid;
    int stat;
    int real_index = 0;
    pid = fork();

    if (pid == 0) {
        pid_t real_pid = getpid();
        printf("%d\n", real_pid);
        add_to_logger(real_pid, line, history, command_num);

        char **log_line ;
        for (int i = 0 ; i <= command_num ; i++) {
            log_line = history[i];
            log_line[1][strlen(log_line[1]) - 1] = 0;
            char *status;
            if (i == command_num) {
                //printf("%d\n", real_index);
                status = RUNNING_STATUS;
            }
            else {
                status = get_status(log_line[0]);
            }
            printf("%s %s %s\n", log_line[0], log_line[1], status);
            fflush(stdout);
        }
        log_line = history[command_num];
        log_line[2] = DONE_STATUS;
        //printf("%s %s %s\n", log_line[0], log_line[1], log_line[2]);
        exit(EXIT_SUCCESS);
    }
    else {
        wait(&stat);
        return pid;
    }
}

pid_t execute_cd(char **args, int last_arg_position) {
    if(last_arg_position > 1) {
        fprintf(stderr, ARGS_NUM_ERROR);
        fflush(stderr);
        return getpid();
    }
    printf("%d\n", getpid());

    if(last_arg_position != 0) {
        if(strcmp(args[1], "~") == 0) {
            struct passwd *pw = getpwuid(getuid());
            const char *homedir = pw->pw_dir;
            args[1] = homedir;
        }
    }
    if(chdir(args[1]) != 0) {
        perror("Error");
    }
    return getpid();
}
/*
 *
 */
pid_t command_execute(char **args, char ***history, char ***jobs, char *line, int command_num) {
    int last_arg_position = get_last_parameter_position(args);
    pid_t child_pid;
    if(args[0] == NULL) {
        //printf("first argument was null");
        return ERROR_CODE;
    }
    if(strcmp(args[0], EXIT_COMMAND) == 0) {
        exit(EXIT_SUCCESS);
    }
    else if(strcmp(args[0], CD_COMMAND) == 0) {
        child_pid = execute_cd(args, last_arg_position);
    }
    else if(strcmp(args[0], HISTORY_COMMAND) == 0) {
        child_pid = execute_history(history, line, command_num);
    }
    else if(strcmp(args[0], JOBS_COMMAND) == 0) {
        child_pid = execute_jobs(jobs);
    }
    // if command needs to run in the background
    else if(*args[last_arg_position] == BACKGROUND) {
        child_pid = execute_background(args, last_arg_position);
        add_to_logger(child_pid, line, jobs, command_num);
    }
    // otherwise command needs to run in the foreground
    else {
        child_pid =  execute_foreground(args);
    }

    // adding command to history (if command is "history", it will be added as part of it's own run)
    if(strcmp(args[0], HISTORY_COMMAND) != 0) {
        add_to_logger(child_pid, line, history, command_num);
    }
    return child_pid;
}

/*
 *
 */
void init_logger_buffer(char ***logger) {
    for(int i=0 ; i < MAX_COMMAND_NUM ; i++) {
        logger[i] = malloc(3 * sizeof(char*));
        logger[i][0] = malloc(sizeof(char*));
        logger[i][1] = malloc(sizeof(char*));
        logger[i][2] = malloc(sizeof(char*));
        logger[i][0] = NULL;
        logger[i][1] = NULL;
        logger[i][2] = NULL;
    }
}

/*
 *
 */
void init_args_buffer(char **args) {
    for(int i=0 ; i < MAX_BUFFER_SIZE ; i++) {
        args[i] = NULL;
    }
}

/*
 *
 */
void free_buffer(char *line, char *args[MAX_BUFFER_SIZE]) {
    free(line);
    for(int i=0 ; i < MAX_BUFFER_SIZE ; i++) {
        free(args[i]);
    }
    //free(args);
}

/*
 *
 */
void command_loop(void) {

    //initializing variables
    char *line;
    int command_status = 1;
    int command_num = 0;
    int *job_num = 0;
    int *command_status_ptr = &command_status;
    char ***history =  malloc(MAX_COMMAND_NUM * sizeof(char**));
    char ***jobs =  malloc(MAX_COMMAND_NUM * sizeof(char**));
    //init_logger_buffer(history);
    //init_logger_buffer(jobs);
    int status = 1;

    //command loop
    do {
        printf(STARTER);

        //reading the command
        line = get_line(command_status_ptr);

        //checking if error occurred
        if (*command_status_ptr == 0) {
            fprintf(stderr, ERROR_MSG);
            continue;
        }

        //initialing argument array
        char **args = malloc(MAX_BUFFER_SIZE * sizeof(char*));
        init_args_buffer(args);

        //parsing the commands arguments into the array
        get_command_args(line, command_status_ptr, args);

        //checking if error occurred
        if (*command_status_ptr == 0) {
            fprintf(stderr, ERROR_MSG);
            continue;
        }

        //executing the command
        pid_t pid = command_execute(args, history, jobs, line, command_num);
        if (pid == ERROR_CODE) {
            continue;
        }

        //free resources
        //free_buffer(line, args);
        command_num++;
        //printf("%d\n", command_num);
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
