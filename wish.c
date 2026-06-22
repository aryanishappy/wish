#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include "input_processing.h"
#include "error_messages.h"
// write(STDERR_FILENO, error_message, strlen(error_message)); 

struct path {
    char* path;
    struct path* next;
};

int main(int argc, char *argv[]) {
    bool flag = 1;
    if(argc > 2) {
        fprintf(stderr, "Incorrect use of command. Possible uses: ./wish <file-name> or ./wish");
        exit(1);
    }
    else if(argc == 2) {
        // for batch mode directing file stream to input stream
        if(freopen(argv[1], "r", stdin) == NULL) {
            fprintf(stderr, "File cannot be opened. Please ensure a valid file in the current directory.");
            exit(1);
        }
        flag = 0;
    }

    // Setting linked list for paths with default starting /bin
    struct path* paths = malloc(sizeof(struct path));
    if(paths == NULL) {
        fprintf(stderr, "Initial memory allocation for path values failed\n");
        return 1;
    }
    paths->path = strdup("/bin");
    paths->next = NULL;
    char* to_free_full_command = NULL;

    // Infinite loop until user enters the command 'exit'
    while(1) {
        //Prompt user for input
        char current_dir[1000];
        if(getcwd(current_dir, 1000) == NULL) {
            fprintf(stderr, "Cannot access current directory name\n");
            exit(1);
        }
        if(flag)    printf("wish: [%s] > ", current_dir);

        // Get the whole input in full_command
        ssize_t input_size = 0;
        free(to_free_full_command);
        char* full_command = take_input(&input_size);
        to_free_full_command = full_command;
        if(full_command == NULL) {
            fprintf(stderr, "Input processing failed. Please retry with proper input.\n");
            continue;
        }

        // stripping whitespace and newline from begin and end
        while(*full_command == ' ') full_command++;
        for(int i = input_size - 2; i >= 0; i--) {
            if(full_command[i] == ' ')  full_command[i] = '\0';
            else                        break;
        }

        char* redirect_ptr = strrchr(full_command, '>');
        if(redirect_ptr != NULL) {
            *redirect_ptr = '\0';
            redirect_ptr++;
            while(*redirect_ptr == ' ') redirect_ptr++;
        }
        char* command = strsep(&full_command, " \t");
        if(command == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message)); 
            continue;
        }
        if(strcmp(command, "exit") == 0) {
            if(full_command == NULL) {
                exit(0);
            }
            else {
                fprintf(stderr, "Invalid command. Possible use: exit\n");
                continue;
            }
        }
        else if(strcmp(command, "cd") == 0) {
            char* argument = strsep(&full_command, " \t");
            if(full_command != NULL || argument == NULL || argument[0] == '\0') {
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                continue;
            }
            if(chdir(argument) != 0) {
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                continue;
            }
        }
        else if(strcmp(command, "path") == 0) {
            while(paths != NULL) {
                struct path* temp = paths;
                paths = paths->next;
                free(temp);
            }
            while(full_command != NULL) {
                struct path* new_path = malloc(sizeof(struct path));
                new_path->path = strsep(&full_command, " ");
                new_path->next = paths;
                paths = new_path;
            }
        }
        else {
            bool found = 0;
            if(access(command, X_OK) == 0) {
                found = 1;
                int ch = fork();
                if(ch < 0) {
                    write(STDERR_FILENO, error_message, strlen(error_message)); 
                }
                else if(ch == 0) {
                    if(redirect_ptr != NULL) {
                        char file_name[101];
                        int fptr = 0;
                        bool to_take_literally = 0, overflow_flag = 0, whitespace_flag = 0;
                        while(*redirect_ptr != '\0') {
                            if(to_take_literally) {
                                if(fptr > 99)  {
                                    overflow_flag = 1;
                                    break;
                                }
                                file_name[fptr++] = *redirect_ptr;
                            }
                            else if(*redirect_ptr == '\\')    to_take_literally = 1;
                            else if(*redirect_ptr == ' ' || *redirect_ptr == '\t') {
                                whitespace_flag = 1;
                                break;
                            }
                            else {
                                if(fptr > 99)  {
                                    overflow_flag = 1;
                                    break;
                                }
                                file_name[fptr++] = *redirect_ptr;
                            }
                        }
                        file_name[fptr] = '\0';
                        if(strlen(file_name) == 0 || overflow_flag || whitespace_flag) {
                            write(STDERR_FILENO, error_message, strlen(error_message)); 
                            break;
                        }
                        FILE* output_file = freopen(file_name, "w", stdout);
                        if(output_file == NULL) {
                            write(STDERR_FILENO, error_message, strlen(error_message)); 
                            break;
                        }
                    }
                    int curr = 1, ct = 1;
                    if(full_command != NULL) {
                        for(int i = 0; i < strlen(full_command); i++) {
                            if(full_command[i] == ' ')  ct++;
                        }
                    }
                    if(ct + 2 > 100) {
                        write(STDERR_FILENO, error_message, strlen(error_message)); 
                        return 1;
                    }
                    char* args[100];
                    args[0] = strdup(command);
                    while(full_command != NULL) {
                        char* temp = strsep(&full_command, " \t");
                        if(temp[0] != '\0') args[curr++] = strdup(temp);
                    }
                    args[curr] = NULL;
                    if(execv(command, args) == -1) {
                        write(STDERR_FILENO, error_message, strlen(error_message)); 
                        return 1;
                    }
                }
                else {
                    wait(NULL);
                }
            }
            else {
                struct path* temp = paths;
                while(temp != NULL) {
                    char* absolute_command = malloc((strlen(command) + strlen(temp->path) + 2) * sizeof(char));
                    if(absolute_command == NULL) {
                        temp = temp->next;
                        continue;
                    }
                    int t = strlen(temp->path);
                    for(int i = 0; i < t; i++) absolute_command[i] = (temp->path)[i];
                    absolute_command[t++] = '/';
                    for(int i = 0; i < strlen(command); i++)    absolute_command[t++] = command[i];
                    absolute_command[t] = '\0';

                    if(access(absolute_command, X_OK) != 0) {
                        temp = temp->next;
                        continue;
                    }
                    found = 1;
                    int ch = fork();
                    if(ch < 0) {
                        write(STDERR_FILENO, error_message, strlen(error_message)); 
                        break;
                    }
                    else if(ch == 0) {
                        if(redirect_ptr != NULL) {
                            char file_name[101];
                            int fptr = 0;
                            bool to_take_literally = 0, overflow_flag = 0, whitespace_flag = 0;
                            while(*redirect_ptr != '\0') {
                                if(to_take_literally) {
                                    if(fptr > 99)  {
                                        overflow_flag = 1;
                                        break;
                                    }
                                    file_name[fptr++] = *redirect_ptr;
                                }
                                else if(*redirect_ptr == '\\')    to_take_literally = 1;
                                else if(*redirect_ptr == ' ' || *redirect_ptr == '\t') {
                                    whitespace_flag = 1;
                                    break;
                                }
                                else {
                                    if(fptr > 99)  {
                                        overflow_flag = 1;
                                        break;
                                    }
                                    file_name[fptr++] = *redirect_ptr;
                                }
                                redirect_ptr++;
                            }
                            file_name[fptr] = '\0';
                            if(strlen(file_name) == 0 || overflow_flag || whitespace_flag) {
                                write(STDERR_FILENO, error_message, strlen(error_message)); 
                                break;
                            }
                            FILE* output_file = freopen(file_name, "w", stdout);
                            if(output_file == NULL) {
                                write(STDERR_FILENO, error_message, strlen(error_message)); 
                                break;
                            }
                        }
                        int curr = 1, ct = 1;
                        if(full_command != NULL) {
                            for(int i = 0; i < strlen(full_command); i++) {
                                if(full_command[i] == ' ')  ct++;
                            }
                        }
                        if(ct + 2 > 100) {
                            write(STDERR_FILENO, error_message, strlen(error_message)); 
                            return 1;
                        }
                        char* args[100];
                        args[0] = strdup(command);
                        while(full_command != NULL) {
                            char* temp = strsep(&full_command, " ");
                            if(temp[0] != '\0') args[curr++] = strdup(temp);
                        }
                        args[curr] = NULL;
                        if(execv(absolute_command, args) == -1) {
                            write(STDERR_FILENO, error_message, strlen(error_message)); 
                            return 1;
                        }
                    }
                    else {
                        wait(NULL);
                    }
                    break;
                }
            }
            if(!found) {
                fprintf(stderr, "Command not found!!\n");
            }
        }
        // free(to_free_full_command);
    }
    free(to_free_full_command);
    return 0;
}