#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <errno.h>

#define MAX_BUFFER_SIZE 100
#define MAX_COMMAND_NUM 100
#define ERROR_CODE -999
#define EXIT_CODE -500
#define BACKGROUND '&'
#define STARTER "> "
#define ERROR_MSG "Error in system call"
#define ARGS_NUM_ERROR "Error: Too many arguments"
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
    // defining the buffer
    size_t buffer_size = 0;
    char *line = NULL;

    // getting the line
    if(getline(&line, &buffer_size, stdin) == -1) {
        if(!feof(stdin)) {
            *command_status_ptr = 0; //didnt get to EOF, therefore read wasn't successful
        }
    }
    return line;
}

/*
 * Parsing the received line into a list of arguments and updates the args array
 */
void get_command_args(char *command, int *command_status_ptr, char *args[MAX_BUFFER_SIZE]) {

    int position = 0;   // position in argument array
    char *arg;          // pointer to current arg
    char *begin;        // args starting position in the command

    // going over the command
    for(;;) {
        // skipping spaces
        while (isspace(*command)) {
            *command++;
        }

        // if we reached the end of the command
        if(*command == '\0') {
            break;
        }

        // if this is a background command
        if(*command == BACKGROUND) {

            // allocating the arg in the args array
            arg = malloc(sizeof(char) + 1);
            strncpy(arg, command, 1);
            arg[strlen(arg)] = '\0';
            args[position] = arg;
            position++;
            *command++;
            arg = NULL;
        }

        // reading a command argument
        if(isalnum(*command) || *command == '-' || *command == '~' || *command == '.') {
            begin = command;
            //todo change to everthing that is not space or /0
            while(isalnum(*command) || *command == '-' || *command == '~' || *command == '.') {
                *command++;
            }

            // allocating the arg in the args array
            arg = malloc((command-begin + 1) * sizeof(char));
            strncpy(arg, begin, command - begin);
            arg[strlen(arg)] = '\0';
            args[position] = arg;
            position++;
            arg = NULL;
        }

        // if we have quotes in the command
        if(*command =='"') {
            int quote = *command++;
            begin = command;

            // going over the command until we reached the closing quote
            while (*command && *command != '"') {
                *command++;
                // if we reached the end of the command, without getting a closing quote, update status to error
                if (*command == '\0') {
                    *command_status_ptr = 0;
                }
            }
            *command++;

            // checking if there is an error, if so finish reading the command
            if (*command_status_ptr == 0) {
                break;
            }

            // allocating the arg in the args array
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
 * removes the last argument from the argument array (used to remove `&` from background commands)
 */
void remove_last_arg(char *args[MAX_BUFFER_SIZE], int last_position) {
    args[last_position] = NULL;
}

/*
 * creates a child process in which the given command will be executed, without waiting for the command to finish
 */
pid_t execute_background(char **args, int last_arg_position) {
    pid_t pid;
    int ret_code = 0;
    remove_last_arg(args, last_arg_position);
    if((pid = fork()) == 0) {
        // child process

        //print pid
        pid_t real_pid = getpid();
        printf("%d\n", real_pid);
        fflush(stdout);

        ret_code = execvp(args[0],args); // executes the command

        // if an error was received
        if(ret_code == -1) {
            // print an error message and exit with failure code
            fprintf(stderr, ERROR_MSG);
            fflush(stderr);
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS); // otherwise exit with success code
    }
    else {
        //parent process

        return pid;
    }
}

/*
 * creates a child process in which the given command will be executed, waits for the command to finish
 */
pid_t execute_foreground(char **args) {
    pid_t pid;
    int stat;
    int ret_code = 0;


    if((pid = fork()) == 0) {
        // child process

        // print pid
        pid_t real_pid = getpid();
        printf("%d\n", real_pid);
        fflush(stdout);

        ret_code = execvp(args[0],args); //executing the command

        // if the command returned an error code
        if(ret_code == -1) {
            // print an error message and return an failiure code
            fprintf(stderr, ERROR_MSG);
            fflush(stderr);
            exit(EXIT_FAILURE);
        }

        // otherwise exit with success code
        exit(EXIT_SUCCESS);
    }
    else {
        // parent process, wait until the child process finish
        wait(&stat);
        sleep(1/2); // sleeping for half a second to make sure STARTER will appear after the commands output
        return pid;
    }
}

/*
 * finds the location of the last arg of the command and returns it
 */
int get_last_parameter_position(char *args[MAX_BUFFER_SIZE]) {
    int last_arg_position = -1;

    // going over the args array until we get null or until we reached the max buffer size
    for(int i=0 ; i < MAX_BUFFER_SIZE - 1; i++) {
        if(args[i + 1] == NULL){
            last_arg_position = i;
            break;
        }
    }

    // if the args array is full
    if(last_arg_position == -1) {
        last_arg_position = MAX_BUFFER_SIZE - 1;
    }
    return last_arg_position;
}

/*
 * gets the status of a command and returns it
 */
char *get_status(const char *pid) {
    int status;
    pid_t received_pid = atoi(pid);
    pid_t return_pid = waitpid(received_pid, &status, WNOHANG); // get the state of the child process
    printf("%d\n", return_pid);
    printf("%d\n", errno == ECHILD);
    printf("%d\n", errno == EINTR);
    printf("%d\n", errno == EINVAL);
    // child is still running
    if (return_pid == 0) {
        return RUNNING_STATUS;
    }

    // child exited or error
    else {
        return DONE_STATUS;
    }
}

/*
 * adds the command's pid and args to a new line in the logger
 */
void add_to_logger(pid_t pid, char *line, char ***logger, int *command_num) {

    // allocating memory of the logger line, then copying the command information to it
    logger[*command_num] = malloc(3 * sizeof(char*));
    logger[*command_num][0] = malloc(sizeof(char*));
    snprintf(logger[*command_num][0],10,"%d", pid);
    logger[*command_num][1] = malloc(sizeof(char*));
    strcpy(logger[*command_num][1], line);
}

/*
 * executes the job command - prints the all the jobs that were started by the program
 */
pid_t execute_jobs(char ***jobs) {
    pid_t pid;
    int stat;
    int real_index = 0;

    if ((pid = fork()) == 0) {
        //child process

        // printing the process's pid
        pid_t real_pid = getpid();
        printf("%d\n", real_pid);

        // going over all the command in the job logger and printing them
        for (int i = 0 ; i < MAX_COMMAND_NUM ; i++) {
            if (jobs[i] == NULL) {
                break;
            }
            char **log_line = jobs[i];
            log_line[1][strlen(log_line[1]) - 1] = 0;                        // removing trailing newline
            printf("%s %s %s\n", log_line[0], log_line[1], log_line[2]); // printing in the appropriate format
            fflush(stdout);
            real_index++;
        }
        exit(EXIT_SUCCESS); // exits with success code
    }
    else {
        // parent process, waiting for the child to finish
        wait(&stat);
        return pid;
    }
}


/*
 * executes the logger command - prints all the commands that were executed by the program
 */
pid_t execute_logger(char ***logger, char *line, int *command_num) {
    pid_t pid;
    int stat;
    pid = fork(); //creating the child process

    if (pid == 0) {
        //child process

        pid_t real_pid = getpid();
        printf("%d\n", real_pid);

        char **log_line ;

        // going over all the commands in the logger and printing them
        for (int i = 0 ; i <= *command_num ; i++) {
            if(logger[i] == NULL)
            {
                break;
            }
            log_line = logger[i];
            log_line[1][strlen(log_line[1]) - 1] = 0;  // removing trailing newline
            printf("%s %s %s\n", log_line[0], log_line[1], log_line[2]); // printing in the appropriate format
        }
        fflush(stdout);
        exit(EXIT_SUCCESS); // exits with success code
    }
    else {
        // parent process, waiting for the child process to finish
        wait(&stat);
        return pid;
    }
}

/*
 * executes the cd command
 */
pid_t execute_cd(char **args, int last_arg_position) {
    // if the command received more the one argument, prints an error and exits
    if(last_arg_position > 1) {
        fprintf(stderr, ARGS_NUM_ERROR);
        fflush(stderr);
        return getpid();
    }
    printf("%d\n", getpid());

    // if the command received parameters
    if(last_arg_position != 0) {
        // and the parameter is ~
        if(strcmp(args[1], "~") == 0) {
            // change the argument to the user's home directory
            struct passwd *pw = getpwuid(getuid());
            const char *homedir = pw->pw_dir;
            args[1] = homedir;
        }
    }
    // if the command didn't execute successfully, return an error and finish the entire program's run
    if(chdir(args[1]) != 0) {
        perror("Error");
        return EXIT_CODE;
    }
    return getpid();
}

void update_process_status(char ***logger) {

    for(int i = 0 ; i < MAX_COMMAND_NUM ; i++) {
        int status;
        if(logger[i] == NULL) {
            break;
        }
        pid_t received_pid = atoi(logger[i][0]);
        pid_t return_pid = waitpid(received_pid, &status, WNOHANG); // get the state of the child process
        // child is still running
        if (return_pid == 0) {
            logger[i][2] = RUNNING_STATUS;
        }

        // child exited or error
        else {
            logger[i][2] =  DONE_STATUS;
        }
    }
}
/*
 * executes the received command and adds it to the relevant logs
 */
pid_t command_execute(char **args, char ***history, char ***jobs, char *line, int *command_num, int *job_num) {
    int last_arg_position = get_last_parameter_position(args); // get the index of the last argument in the args array
    pid_t child_pid;

    // if the user pressed enter
    if(args[0] == NULL) {
        return ERROR_CODE;
    }

    // if the user entered the exit command
    if(strcmp(args[0], EXIT_COMMAND) == 0) {
        return EXIT_CODE;
    }

    // if the user entered the cd command
    else if(strcmp(args[0], CD_COMMAND) == 0) {
        child_pid = execute_cd(args, last_arg_position);
    }

    // if the user entered the history command
    else if(strcmp(args[0], HISTORY_COMMAND) == 0) {
        update_process_status(history);
        child_pid = execute_logger(history, line, command_num);
        // printing the history command as the last line in the history log
        printf("%d %s %s\n", child_pid, HISTORY_COMMAND, RUNNING_STATUS); // printing in the appropriate format
    }

    // ih the user entered the jobs command
    else if(strcmp(args[0], JOBS_COMMAND) == 0) {
        update_process_status(jobs);
        child_pid = execute_logger(jobs, line, command_num);
    }
    // if command needs to run in the background, run it and add it to the jobs logger
    else if(*args[last_arg_position] == BACKGROUND) {
        child_pid = execute_background(args, last_arg_position);
        add_to_logger(child_pid, line, jobs, job_num);
        (*job_num)++;
        //printf("job num %d\n", *job_num);
    }
    // otherwise command needs to run in the foreground
    else {
        child_pid = execute_foreground(args);
    }

    // adding command to history
    add_to_logger(child_pid, line, history, command_num);
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
 * setting all the arg pointers to null
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
 * The main command loop - This function runs command from the user, until an exit code is received  - either
 * if the user sent the exit command, of if the cd command failed
 */
void command_loop(void) {

    //initializing variables
    char *line;
    int command_status = 1;
    int command_num = 0;
    int job_num = 0;
    int *command_status_ptr = &command_status;
    char ***history =  malloc(MAX_COMMAND_NUM * sizeof(char**));
    char ***jobs =  malloc(MAX_COMMAND_NUM * sizeof(char**));
    //init_logger_buffer(history);
    //init_logger_buffer(jobs);
    pid_t pid = 0;

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
        pid = command_execute(args, history, jobs, line, &command_num, &job_num);
        if (pid == ERROR_CODE) {
            continue;
        }

        //free resources
        //free_buffer(line, args);
        command_num++;
        //printf("%d\n", command_num);
    } while(pid != EXIT_CODE);

    free(history);
    free(jobs);
    exit(EXIT_SUCCESS);
}

/*
 * The main calls the command_loop function which controls most of the programs logic
 */
int main() {
    //running the command loop
    command_loop();

    return 0;
}
