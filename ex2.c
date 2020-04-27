#include <stdio.h>
#include <stdlib.h>

char *get_command(void) {

}

char **get_args(char *command) {

}
void command_loop(void) {
    char *command;
    char **command_args;
    int status;

    do {
        printf("> ");
        command = get_command();
        command_args = get_args(command);

        free(command);
        free(command_args);
    } while(status);
}

int main() {
    command_loop();
    return 0;
}
