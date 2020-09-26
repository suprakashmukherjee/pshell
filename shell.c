#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void main_loop();
char* read_command();
char** parse_command(char*);
int shell_exec(char**);

int main(int argc, char **argv) {
    main_loop();
    return EXIT_SUCCESS;
}

void main_loop() {
    char* command;
    char** parsed_commands;
    int status = 1;

    while(status) {
        printf("pshell > ");
        fflush(stdout);
        command = read_command();
        printf("\n");
        printf("%s\n", command);
        parsed_commands = parse_command(command);
        status = shell_exec(parsed_commands);
    }

    free(command);
    free(parsed_commands);
}

char* read_command() {
    char* input = NULL;
    ssize_t bufsize = 0;

    if(getline(&input, &bufsize, stdin) == -1) {
        if(feof(stdin)) {
            exit(EXIT_SUCCESS);
        }
        else {
            perror("pshell: ERR: read_command");
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
    }

    return input;
}

char** parse_command(char* command) {
    return NULL;
}

int shell_exec(char** parse_command) {
    return 1;
}