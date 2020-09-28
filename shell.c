//
//  Some Comments
//
//
//

#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define MAX_ARGS 10
#define MAX_ARGS_LEN 15
#define MAX_PATH_LIST 256
#define MAX_TOKEN_SIZE 256

static int num_args_type_np = 0;
static int num_args_type_dp = 0;
static int num_args_type_tp = 0;
static int num_args_type_sp = 0;

static char* command_type = NULL;
static char* PATH = NULL;

void main_loop();
char* read_command();
char** parse_command(char*);
int shell_exec(char**, char*);
void print(char**, int);
void string_tokenizer(char*, char**, char*, int*);
char* trimwhitespace(char*);


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
    input = trimwhitespace(input);
    // printf("%d\n", strlen(input));
    return input;
}

char* trimwhitespace(char *str) {
    char* end;
    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)
        return str;

    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    return str;
}


char** parse_command(char* command) {
    char** parsed_command;
    parsed_command = (char**) calloc(MAX_ARGS+1, sizeof(char *));

    if(strstr(command, "|") != NULL) {
        command_type = "SP";
        string_tokenizer(command, parsed_command, "|", &num_args_type_sp);
    }
    else if(strstr(command, "||") != NULL) {
        command_type = "DP";
        string_tokenizer(command, parsed_command, "||", &num_args_type_dp);
    }
    else if(strstr(command, "|||") != NULL) {
        command_type = "TP";
        string_tokenizer(command, parsed_command, "|||", &num_args_type_tp);
    }
    else {
        command_type = "NP";
        string_tokenizer(command, parsed_command, " ", &num_args_type_np);
    }
    return parsed_command;
}

void string_tokenizer(char* command, char** parsed_command, char* delim, int* num_args) {
    int i = 0;
    char* token = strtok(command, delim);
    while(token != NULL) {
        parsed_command[i] = (char*) calloc(strlen(token), sizeof(char));
        parsed_command[i] = token;
        i++;
        token = strtok(NULL, delim);
    }
    *num_args = i;
}

int shell_exec(char** parsed_command, char* type) {
    // print(parsed_command, num_args_type_np);

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
        // print(path_list, num_args_type_np);
        pid_t childpid;
        int status;
        i = 0;
        
        while(path_list[i] != NULL) {
            printf("************************************************************\n");
            char* main_comm = parsed_command[0];
            char* buff = (char*) calloc(MAX_TOKEN_SIZE, sizeof(char));
            strcpy(buff, path_list[i]);
            strcat(buff, "/");
            strcat(buff, parsed_command[0]);
    
            // printf("......%s\n", buff);
            parsed_command[0] = buff;
            if(access(buff, R_OK) == 0) {

                if((childpid = fork()) == -1) {
                perror("pshell : ERR : Command Type NP Cannot fork\n");
                exit(1);
                }
                else if(childpid == 0) {
                    printf("Executing %s\n", buff);
                    // print(parsed_command, num_args_type_np);
                    
                    char* args_vec[num_args_type_np+1];
                    if(num_args_type_np == 1) {
                        args_vec[0] = parsed_command[0];
                        args_vec[1] = NULL;
                    }
                    else {
                        for(int j = 0; j < num_args_type_np+1; j++) {
                            args_vec[j] = parsed_command[j];
                        }
                        args_vec[num_args_type_np] = NULL;
                    }
                    
                    if(execv(buff, args_vec) < 0) {
                        printf("pshell : ERRNO in exec : %d\n", errno);
                    }
                }
                else {
                    while(wait(&status) != childpid);
                    printf("Command Successful\n");
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
            printf("************************************************************\n");
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

void print(char** parse_command, int num) {
   for(int i = 0; i < num; i++) {
       printf("%s\n", *(&parse_command[i]));
   }
    // fflush(stdout);
}