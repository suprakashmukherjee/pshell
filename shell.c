#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define MAX_ARGS 10
#define MAX_ARGS_LEN 15
#define MAX_PATH_LIST 256
#define MAX_TOKEN_SIZE 256

static int num_args_type1 = 0;
static char* command_type = NULL;
static char* PATH = NULL;

void main_loop();
char* read_command();
char** parse_command(char*);
int shell_exec(char**, char*);
void print(char**);

int main(int argc, char **argv) {
    main_loop();
    return EXIT_SUCCESS;
}

void main_loop() {
    char* command;
    char** parsed_commands;
    int status = 1;
    PATH = getenv("PATH");
    while(status) {
        printf("pshell > ");
        fflush(stdout);
        command = read_command();
        parsed_commands = parse_command(command);
        status = shell_exec(parsed_commands, command_type);
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
            perror("pshell: ERR: read_command\n");
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
    }

    return input;
}

char** parse_command(char* command) {
    char** parsed_command;
    parsed_command = (char**) calloc(MAX_ARGS+1, sizeof(char *));

    if(strstr(command, "|") != NULL) {
        command_type = "SP";

    }
    else if(strstr(command, "||") != NULL) {
        command_type = "DP";

    }
    else if(strstr(command, "|||") != NULL) {
        command_type = "TP";
    }
    else {
        command_type = "NP";
        int i = 0;
        char* token = strtok(command, " ");
        while(token != NULL) {
            parsed_command[i] = (char*) calloc(strlen(token), sizeof(char));
            parsed_command[i] = token;
            i++;
            token = strtok(NULL, " ");
        }
        num_args_type1 = i;
    }

    return parsed_command;
}

int shell_exec(char** parsed_command, char* type) {
    // print(parsed_command);

    if(strcmp(type, "NP") == 0) {
        char** path_list = (char**) calloc(MAX_PATH_LIST+1, sizeof(char *));

        char* path = (char *) calloc(strlen(PATH), sizeof(char));
        strcpy(path, PATH);
        int i = 0;
        char* token = strtok(path, ":");
        while(token != NULL) {
            path_list[i] = (char*) calloc(MAX_TOKEN_SIZE, sizeof(char));
            path_list[i] = token;
            i++;
            token = strtok(NULL, ":");
        }
        print(path_list);
        pid_t childpid;
        int status;
        i = 0;
        
        while(path_list[i] != NULL) {
            char* main_comm = parsed_command[0];
            char* buff = (char*) calloc(MAX_TOKEN_SIZE, sizeof(char));
            strcpy(buff, path_list[i]);
            strcat(buff, "/");
            strcat(buff, parsed_command[0]);
    
            printf("......%s\n", buff);
            parsed_command[0] = buff;
            if(access(buff, X_OK) == 0) {

                if((childpid = fork()) == -1) {
                perror("pshell : ERR : Command Type NP Cannot fork\n");
                exit(1);
                }
                else if(childpid == 0) {
                    printf("Executing %s\n", buff);
                    // print(parsed_command);

                    char* args_vec[num_args_type1+1];
                    int j;
                    for(j = 0; j < num_args_type1+1; j++) {
                        args_vec[j] = parsed_command[j];
                    }
                    //remove extra newline character
                    args_vec[j-2][strlen(args_vec[j-2])-1] = NULL;
                    args_vec[num_args_type1] = NULL;
                    // print_arr(args_vec);
                    
                    if(execv(buff, args_vec) < 0) {
                        printf("pshell : ERRNO in exec : %d\n", errno);
                    }
                }
                else {
                    while(wait(&status) != childpid);
                    printf("exit\n");
                }
            }
            else {
                if (errno == ENOENT) 
                    printf ("%s does not exist\n", buff);
                else if (errno == EACCES) 
                    printf ("%s is not accessible\n", buff);
            }
            parsed_command[0] = main_comm;
            free(buff);
            i++;
        }
        free(path_list);
        free(path);
    }
    else if(strcmp(type, "SP") == 0) {

    }
    else if(strcmp(type, "DP") == 0) {

    }
    else if(strcmp(type, "TP") == 0) {

    }
    else {
        printf("pshell : ERR : Command not recognized");
        fflush(stdout);
    }
    return 1;
}

void print(char** parse_command) {
   for(int i = 0; i < num_args_type1; i++) {
       printf("%s\n", parse_command[i]);
   }
    // fflush(stdout);
}

void print_arr(char* arr[]) {
    for(int i = 0; i < num_args_type1+1; i++) {
        printf("-%s-\t", arr[i]);
    }
}