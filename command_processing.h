#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "error_messages.h"

struct path {
    char* path;
    struct path* next;
};
struct path* paths = NULL;

int path_initialization() {
    paths = (struct path *) malloc(sizeof(struct path));
    if(paths == NULL) {
        fprintf(stderr, "Initial memory allocation for path values failed\n");
        return 1;
    }
    paths->path = strdup("/bin");
    paths->next = NULL;
    return 0;
}

int process(char* full_command) {
    int input_size = strlen(full_command);
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
        return 1;
    }

    if(strcmp(command, "exit") == 0) {
        if(full_command == NULL) {
            exit(0);
        }
        else {
            fprintf(stderr, "Invalid command. Possible use: exit\n");
            return 1;
        }
    }
    else if(strcmp(command, "cd") == 0) {
        char* argument = strsep(&full_command, " \t");
        if(full_command != NULL || argument == NULL || argument[0] == '\0') {
            write(STDERR_FILENO, error_message, strlen(error_message)); 
            return 1;
        }
        if(chdir(argument) != 0) {
            write(STDERR_FILENO, error_message, strlen(error_message)); 
            return 1;
        }
    }
    else if(strcmp(command, "path") == 0) {
        while(paths != NULL) {
            struct path* temp = paths;
            paths = paths->next;
            free(temp);
        }
        while(full_command != NULL) {
            struct path* new_path = (struct path *) malloc(sizeof(struct path));
            new_path->path = strsep(&full_command, " ");
            new_path->next = paths;
            paths = new_path;
        }
    }
    else {
        struct path* temp = (struct path *) malloc(sizeof(struct path));
        if((temp->path = get_current_dir_name()) == NULL) {
            fprintf(stderr, "Failed to get current directory for executables\n");
            return 1;
        }
        temp->next = paths;

        while(temp != NULL) {
            char* absolute_command = (char *) malloc((strlen(command) + strlen(temp->path) + 2) * sizeof(char));
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
            int ch = fork();
            if(ch < 0) {
                write(STDERR_FILENO, error_message, strlen(error_message)); 
                return 1;
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
                                return 1;
                            }
                            file_name[fptr++] = *redirect_ptr;
                        }
                        else if(*redirect_ptr == '\\')    to_take_literally = 1;
                        else if(*redirect_ptr == ' ' || *redirect_ptr == '\t') {
                            whitespace_flag = 1;
                            return 1;
                        }
                        else {
                            if(fptr > 99)  {
                                overflow_flag = 1;
                                return 1;
                            }
                            file_name[fptr++] = *redirect_ptr;
                        }
                        redirect_ptr++;
                    }
                    file_name[fptr] = '\0';
                    if(strlen(file_name) == 0 || overflow_flag || whitespace_flag) {
                        write(STDERR_FILENO, error_message, strlen(error_message)); 
                        return 1;
                    }
                    FILE* output_file = freopen(file_name, "w", stdout);
                    if(output_file == NULL) {
                        write(STDERR_FILENO, error_message, strlen(error_message)); 
                        return 1;
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
                return 0;
            }
            else {
                wait(NULL);
                return 0;
            }
            break;
        }
        return 2;
    }
    return 3;
}
