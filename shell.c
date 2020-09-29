//
//  Some Comments
//
//
//

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_ARGS 10
#define MAX_ARGS_LEN 128
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
        if(strlen(command) == 0) {
            continue;
        }
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
            printf("pshell: ERR %d: read_command\n", errno);
            fflush(stdout);
            exit(EXIT_FAILURE);
        }
    }
    input = trimwhitespace(input);
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

    if(strcmp(command, "exit") == 0) {
        exit(EXIT_SUCCESS);
    }
    if(strcmp(command, "EXIT") == 0) {
        exit(EXIT_SUCCESS);
    }

    if(strstr(command, "|||") != NULL) {
        command_type = "TP";
        string_tokenizer(command, parsed_command, "|||", &num_args_type_tp);
    }
    else if(strstr(command, "||") != NULL) {
        command_type = "DP";
        string_tokenizer(command, parsed_command, "||", &num_args_type_dp);
    }
    else if(strstr(command, "|") != NULL) {
        command_type = "SP";
        string_tokenizer(command, parsed_command, "|", &num_args_type_sp);
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
    
            parsed_command[0] = buff;
            if(access(buff, R_OK) == 0) {

                if((childpid = fork()) == -1) {
                printf("pshell : ERR %d : Command Type NP Cannot fork\n", errno);
                exit(1);
                }
                else if(childpid == 0) {
                    printf("Executing %s\n", buff);
                    
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
                        printf("pshell : ERR %d : Cannot exec command\n", errno);
                    }
                }
                else {
                    while(wait(&status) != childpid);
                    printf("Command Successful : PID %d\n", childpid);
                    printf("Status: %d\n", status);
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
    
        char* left_pipe = parsed_command[0];
        char* right_pipe = parsed_command[1];
        char** parsed_right_pipe;
        int num;
        parsed_right_pipe = (char**) calloc(MAX_ARGS+1, sizeof(char *));
        string_tokenizer(right_pipe, parsed_right_pipe, ",", &num);
        char* first_program = parsed_right_pipe[0];
        char* second_program = parsed_right_pipe[1];

        if (mkfifo ("/tmp/fifo1", S_IRUSR| S_IWUSR) < 0) {
            printf("pshell : ERR %d: Error creating FIFO\n", errno);
            return 1;
        }

        switch(fork()) {
            case -1:
                printf("pshell : ERR %d : Error in fork first prgm\n", errno);
            case 0: {
                int fd;
                fd = open("/tmp/fifo1", O_RDONLY);

                if(fd != STDIN_FILENO) {
                    if(dup2(fd, STDIN_FILENO) == -1) {
                        printf("pshell : ERR %d : Error in dup read end in first prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                    if(close(fd) == -1) {
                        printf("pshell : ERR %d : Error in closing read end in first prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }

                char** first_prog_args;
                int num_first_args_right;
                first_prog_args = (char**) calloc(MAX_ARGS+1, sizeof(char *));
                string_tokenizer(first_program, first_prog_args, " ", &num_first_args_right);
                
                char* args_vec[num_first_args_right+1];
                if(num_first_args_right == 1) {
                    args_vec[0] = first_prog_args[0];
                    args_vec[1] = NULL;
                }
                else {
                    for(int j = 0; j < num_first_args_right+1; j++) {
                        args_vec[j] = first_prog_args[j];
                    }
                    args_vec[num_first_args_right] = NULL;
                }
                // printf("Executing %s\n", first_program);
                execvp(first_prog_args[0], args_vec);
                printf("pshell : ERR %d : error in exec in first program\n", errno);
                exit(EXIT_FAILURE);
            }
            default:
                break;
        }

        int f_pfd[2];
        if(pipe(f_pfd) == -1) {
            printf("pshell : ERR %d: Error in Pipe\n", errno);
        }
        
        switch(fork()) {
            case -1:
                printf("pshell : ERR %d : Error in Fork for DP Command type\n", errno);
            case 0: {
                if(close(f_pfd[0]) == -1) {
                    printf("pshell : ERR %d : Error in closing read fd for left prgm\n", errno);
                    exit(EXIT_FAILURE);
                }
                
                if(f_pfd[1] != STDOUT_FILENO) {
                    if(dup2(f_pfd[1], STDOUT_FILENO) == -1) {
                        printf("pshell : ERR %d : Cannot dup write end in left prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                    if(close(f_pfd[1]) == -1) {
                        printf("pshell : ERR %d : cannot close write end in left prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }
                
                char** left_pipe_args;
                int num_args_left;
                left_pipe_args = (char**) calloc(MAX_ARGS+1, sizeof(char *));
                string_tokenizer(left_pipe, left_pipe_args, " ", &num_args_left);
                
                char* args_vec[num_args_left+1];
                if(num_args_left == 1) {
                    args_vec[0] = left_pipe_args[0];
                    args_vec[1] = NULL;
                }
                else {
                    for(int j = 0; j < num_args_left+1; j++) {
                        args_vec[j] = left_pipe_args[j];
                    }
                    args_vec[num_args_left] = NULL;
                }

                // printf("Executing %s\n", left_pipe);
                execvp(left_pipe_args[0], args_vec);
                printf("pshell : ERR %d : cannot exec left prgm\n", errno);
                exit(EXIT_FAILURE);
            }
            default:
                break;
        }

        int s_pfd[2];
        if(pipe(s_pfd) == -1) {
            printf("pshell : ERR %d: Error in Pipe\n", errno);
        }

        switch(fork()) {
            case -1:
                printf("pshell : ERR %d: Error in fork tee prgm\n", errno);
            case 0: {
                if(close(f_pfd[1]) == -1) {
                    printf("pshell : ERR %d: cannot close f_pfd write end in tee prgm\n", errno);
                    exit(EXIT_FAILURE);
                }
                if(close(s_pfd[0]) == -1) {
                    printf("pshell : ERR %d: cannot close s_pfd write end in tee prgm\n", errno);
                    exit(EXIT_FAILURE);
                }
                
                if(f_pfd[0] != STDIN_FILENO) {
                    if(dup2(f_pfd[0], STDIN_FILENO) == -1) {
                        printf("pshell : ERR %d: Error in f_pfd dup read end in tee prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                    if(close(f_pfd[0]) == -1) {
                        printf("pshell : ERR %d: Error in f_pfd closing read end in tee prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }
                
                if(s_pfd[1] != STDOUT_FILENO) {
                    if(dup2(s_pfd[1], STDOUT_FILENO) == -1) {
                        printf("pshell : ERR %d: Error in s_pfd dup read end in tee prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                    if(close(s_pfd[1]) == -1) {
                        printf("pshell : ERR %d: Error in s_pfd closing read end in tee prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }

                printf("Executing Tee\n");
                execlp("tee", "tee", "/tmp/fifo1", NULL);
                printf("pshell : ERR %d : error in exec in tee program\n", errno);
                exit(EXIT_FAILURE);
            }
            default:
                break;
        }

        if(close(f_pfd[0]) == -1) {
            printf("pshell : ERR %d : error closing read end of first pipe Main\n", errno);
        }
        if(close(f_pfd[1]) == -1) {
            printf("pshell : ERR %d : error closing write end of first pipe Main\n", errno);
        }
        if(wait(NULL) == -1) {
            printf("pshell : ERR %d : waiting for child Main\n", errno);
        }
        if(wait(NULL) == -1) {
            printf("pshell : ERR %d : waiting for child Main\n", errno);
        }
        if(wait(NULL) == -1) {
            printf("pshell : ERR %d : waiting for child Main\n", errno);
        }
        
        switch(fork()) {
            case -1:
                printf("pshell : ERR %d : Error in fork second prgm\n", errno);
            case 0: {
                if(close(s_pfd[1]) == -1) {
                    printf("pshell : ERR %d: cannot close s_pfd write end in tee prgm\n", errno);
                    exit(EXIT_FAILURE);
                }
                
                if(s_pfd[0] != STDIN_FILENO) {
                    if(dup2(s_pfd[0], STDIN_FILENO) == -1) {
                        printf("pshell : ERR %d : Error in dup read end in second prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                    if(close(s_pfd[0]) == -1) {
                        printf("pshell : ERR %d : Error in closing read end in second prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }
                char** second_prog_args;
                int num_second_args_right;
                second_prog_args = (char**) calloc(MAX_ARGS+1, sizeof(char *));
                string_tokenizer(second_program, second_prog_args, " ", &num_second_args_right);
                
                char* args_vec[num_second_args_right+1];
                if(num_second_args_right == 1) {
                    args_vec[0] = second_prog_args[0];
                    args_vec[1] = NULL;
                }
                else {
                    for(int j = 0; j < num_second_args_right+1; j++) {
                        args_vec[j] = second_prog_args[j];
                    }
                    args_vec[num_second_args_right] = NULL;
                }
                // printf("Executing %s\n", second_program);
                execvp(second_prog_args[0], args_vec);
                printf("pshell : ERR %d : error in exec in second program\n", errno);
                exit(EXIT_FAILURE);
            }
            default:
                break;
        }
        
        if(close(s_pfd[0]) == -1) {
            printf("pshell : ERR %d : error closing read end of second pipe Main\n", errno);
        }
        if(close(s_pfd[1]) == -1) {
            printf("pshell : ERR %d : error closing write end of second pipe Main\n", errno);
        }
        if(wait(NULL) == -1) {
            printf("pshell : ERR %d : waiting for child Main\n", errno);
        }

        switch(fork()) {
            case -1:
                printf("pshell : ERR %d : Error in Fork for DP Command type\n", errno);
            case 0:
                execlp("rm", "rm", "-r", "/tmp/fifo1", NULL);
                printf("pshell : ERR %d : Cannot delete FIFO\n", errno);
                exit(EXIT_FAILURE);
            default:
                break;
        }
        if(wait(NULL) == -1) {
            printf("pshell : ERR %d : waiting for child Main\n", errno);
        }

    }
    else if(strcmp(type, "TP") == 0) {
        char* left_pipe = parsed_command[0];
        char* right_pipe = parsed_command[1];
        char** parsed_right_pipe;
        int num;
        parsed_right_pipe = (char**) calloc(MAX_ARGS+1, sizeof(char *));
        string_tokenizer(right_pipe, parsed_right_pipe, ",", &num);
        char* first_program = parsed_right_pipe[0];
        char* second_program = parsed_right_pipe[1];
        char* third_program = parsed_right_pipe[2];

        if (mkfifo ("/tmp/fifo1", S_IRUSR| S_IWUSR) < 0) {
            printf("pshell : ERR %d: Error creating FIFO1\n", errno);
            return 1;
        }
        if (mkfifo ("/tmp/fifo2", S_IRUSR| S_IWUSR) < 0) {
            printf("pshell : ERR %d: Error creating FIFO2\n", errno);
            return 1;
        }

        switch(fork()) {
            case -1:
                printf("pshell : ERR %d: Error in fork first prgm\n", errno);
            case 0: {
                int fd;
                fd = open("/tmp/fifo1", O_RDONLY);

                if(fd != STDIN_FILENO) {
                    if(dup2(fd, STDIN_FILENO) == -1) {
                        printf("pshell : ERR %d : Error in dup read end in first prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                    if(close(fd) == -1) {
                        printf("pshell : ERR %d : Error in closing read end in first prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }

                char** first_prog_args;
                int num_first_args_right;
                first_prog_args = (char**) calloc(MAX_ARGS+1, sizeof(char *));
                string_tokenizer(first_program, first_prog_args, " ", &num_first_args_right);
                
                char* args_vec[num_first_args_right+1];
                if(num_first_args_right == 1) {
                    args_vec[0] = first_prog_args[0];
                    args_vec[1] = NULL;
                }
                else {
                    for(int j = 0; j < num_first_args_right+1; j++) {
                        args_vec[j] = first_prog_args[j];
                    }
                    args_vec[num_first_args_right] = NULL;
                }
                // printf("Executing %s\n", first_program);
                execvp(first_prog_args[0], args_vec);
                printf("pshell : ERR %d : error in exec in first program\n", errno);
                exit(EXIT_FAILURE);
            }
            default:
                break;
        }

        switch(fork()) {
            case -1:
                printf("pshell : ERR %d: Error in fork second prgm\n", errno);
            case 0: {
                int fd;
                fd = open("/tmp/fifo2", O_RDONLY);

                if(fd != STDIN_FILENO) {
                    if(dup2(fd, STDIN_FILENO) == -1) {
                        printf("pshell : ERR %d : Error in dup read end in second prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                    if(close(fd) == -1) {
                        printf("pshell : ERR %d : Error in closing read end in second prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }

                char** second_prog_args;
                int num_second_args_right;
                second_prog_args = (char**) calloc(MAX_ARGS+1, sizeof(char *));
                string_tokenizer(second_program, second_prog_args, " ", &num_second_args_right);
                
                char* args_vec[num_second_args_right+1];
                if(num_second_args_right == 1) {
                    args_vec[0] = second_prog_args[0];
                    args_vec[1] = NULL;
                }
                else {
                    for(int j = 0; j < num_second_args_right+1; j++) {
                        args_vec[j] = second_prog_args[j];
                    }
                    args_vec[num_second_args_right] = NULL;
                }
                // printf("Executing %s\n", second_program);
                execvp(second_prog_args[0], args_vec);
                printf("pshell : ERR %d : error in exec in second program\n", errno);
                exit(EXIT_FAILURE);
            }
            default:
                break;
        }

        int f_pfd[2];
        if(pipe(f_pfd) == -1) {
            printf("pshell : ERR %d: Error in Pipe\n", errno);
        }
        
        switch(fork()) {
            case -1:
                printf("pshell : ERR %d: Error in Fork for DP Command type\n", errno);
            case 0: {
                if(close(f_pfd[0]) == -1) {
                    printf("pshell : ERR %d : Error in closing read fd for left prgm\n", errno);
                    exit(EXIT_FAILURE);
                }
                
                if(f_pfd[1] != STDOUT_FILENO) {
                    if(dup2(f_pfd[1], STDOUT_FILENO) == -1) {
                        printf("pshell : ERR %d : Cannot dup write end in left prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                    if(close(f_pfd[1]) == -1) {
                        printf("pshell : ERR %d : cannot close write end in left prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }
                
                char** left_pipe_args;
                int num_args_left;
                left_pipe_args = (char**) calloc(MAX_ARGS+1, sizeof(char *));
                string_tokenizer(left_pipe, left_pipe_args, " ", &num_args_left);
                
                char* args_vec[num_args_left+1];
                if(num_args_left == 1) {
                    args_vec[0] = left_pipe_args[0];
                    args_vec[1] = NULL;
                }
                else {
                    for(int j = 0; j < num_args_left+1; j++) {
                        args_vec[j] = left_pipe_args[j];
                    }
                    args_vec[num_args_left] = NULL;
                }

                // printf("Executing %s\n", left_pipe);
                execvp(left_pipe_args[0], args_vec);
                printf("pshell : ERR %d : cannot exec left prgm\n", errno);
                exit(EXIT_FAILURE);
            }
            default:
                break;
        }

        int s_pfd[2];
        if(pipe(s_pfd) == -1) {
            printf("pshell : ERR %d: Error in Pipe\n", errno);
        }

        switch(fork()) {
            case -1:
                printf("pshell : ERR %d: Error in fork tee prgm\n", errno);
            case 0: {
                if(close(f_pfd[1]) == -1) {
                    printf("pshell : ERR %d: cannot close f_pfd write end in tee prgm\n", errno);
                    exit(EXIT_FAILURE);
                }
                if(close(s_pfd[0]) == -1) {
                    printf("pshell : ERR %d: cannot close s_pfd write end in tee prgm\n", errno);
                    exit(EXIT_FAILURE);
                }
                
                if(f_pfd[0] != STDIN_FILENO) {
                    if(dup2(f_pfd[0], STDIN_FILENO) == -1) {
                        printf("pshell : ERR %d: Error in f_pfd dup read end in tee prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                    if(close(f_pfd[0]) == -1) {
                        printf("pshell : ERR %d: Error in f_pfd closing read end in tee prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }
                
                if(s_pfd[1] != STDOUT_FILENO) {
                    if(dup2(s_pfd[1], STDOUT_FILENO) == -1) {
                        printf("pshell : ERR %d: Error in s_pfd dup read end in tee prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                    if(close(s_pfd[1]) == -1) {
                        printf("pshell : ERR %d: Error in s_pfd closing read end in tee prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }

                printf("Executing Tee\n");
                execlp("tee", "tee", "/tmp/fifo1", "/tmp/fifo2", NULL);
                printf("pshell : ERR %d : error in exec in tee program\n", errno);
                exit(EXIT_FAILURE);
            }
            default:
                break;
        }

        if(close(f_pfd[0]) == -1) {
            printf("pshell : ERR %d : error closing read end of first pipe Main\n", errno);
        }
        if(close(f_pfd[1]) == -1) {
            printf("pshell : ERR %d : error closing write end of first pipe Main\n", errno);
        }
        if(wait(NULL) == -1) {
            printf("pshell : ERR %d : waiting for child Main\n", errno);
        }
        if(wait(NULL) == -1) {
            printf("pshell : ERR %d : waiting for child Main\n", errno);
        }
        if(wait(NULL) == -1) {
            printf("pshell : ERR %d : waiting for child Main\n", errno);
        }
        if(wait(NULL) == -1) {
            printf("pshell : ERR %d : waiting for child Main\n", errno);
        }
        
        switch(fork()) {
            case -1:
                printf("pshell : ERR %d : Error in fork third prgm\n", errno);
            case 0: {
                if(close(s_pfd[1]) == -1) {
                    printf("pshell : ERR %d: cannot close s_pfd write end in third prgm\n", errno);
                    exit(EXIT_FAILURE);
                }
                
                if(s_pfd[0] != STDIN_FILENO) {
                    if(dup2(s_pfd[0], STDIN_FILENO) == -1) {
                        printf("pshell : ERR %d : Error in dup read end in third prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                    if(close(s_pfd[0]) == -1) {
                        printf("pshell : ERR %d: Error in closing read end in third prgm\n", errno);
                        exit(EXIT_FAILURE);
                    }
                }
                char** third_prog_args;
                int num_third_args_right;
                third_prog_args = (char**) calloc(MAX_ARGS+1, sizeof(char *));
                string_tokenizer(third_program, third_prog_args, " ", &num_third_args_right);
                
                char* args_vec[num_third_args_right+1];
                if(num_third_args_right == 1) {
                    args_vec[0] = third_prog_args[0];
                    args_vec[1] = NULL;
                }
                else {
                    for(int j = 0; j < num_third_args_right+1; j++) {
                        args_vec[j] = third_prog_args[j];
                    }
                    args_vec[num_third_args_right] = NULL;
                }
                // printf("Executing %s\n", third_program);
                execvp(third_prog_args[0], args_vec);
                printf("pshell : ERR %d: error in exec in third program\n", errno);
                exit(EXIT_FAILURE);
            }
            default:
                break;
        }
        
        if(close(s_pfd[0]) == -1) {
            printf("pshell : ERR %d: error closing read end of second pipe Main\n", errno);
        }
        if(close(s_pfd[1]) == -1) {
            printf("pshell : ERR %d: error closing write end of second pipe Main\n", errno);
        }
        if(wait(NULL) == -1) {
            printf("pshell : ERR %d: waiting for child Main\n", errno);
        }

        switch(fork()) {
            case -1:
                printf("pshell : ERR %d: Error in Fork for DP Command type\n", errno);
            case 0:
                execlp("rm", "rm", "-r", "/tmp/fifo1", NULL);
                printf("pshell : ERR %d: Cannot delete FIFO1\n", errno);
                exit(EXIT_FAILURE);
            default:
                break;
        }
        if(wait(NULL) == -1) {
            printf("pshell : ERR %d: waiting for child Main\n", errno);
        }

        switch(fork()) {
            case -1:
                printf("pshell : ERR %d: Error in Fork for DP Command type\n", errno);
            case 0:
                execlp("rm", "rm", "-r", "/tmp/fifo2", NULL);
                printf("pshell : ERR %d: Cannot delete FIFO2\n", errno);
                exit(EXIT_FAILURE);
            default:
                break;
        }
        if(wait(NULL) == -1) {
            printf("pshell : ERR %d: waiting for child Main\n", errno);
        }

    }
    else {
        printf("pshell : ERR %d: Command not recognized\n", errno);
        fflush(stdout);
    }
    return 1;
}

void print(char** parse_command, int num) {
   for(int i = 0; i < num; i++) {
       printf("%s\n", *(&parse_command[i]));
   }
}