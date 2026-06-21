#ifndef INPUT_PROCESSING_H
#define INPUT_PROCESSING_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>

#include "error_messages.h"

char* take_input(ssize_t* final_input_size) {
    char* input_line = NULL;
    char* final_command = (char *) malloc(sizeof(char));
    if(final_command == NULL) {
        write(STDERR_FILENO, malloc_error_message, strlen(malloc_error_message));
        return NULL;
    }
    *final_command = '\0';
    int size_of_final_command = 1;
    char* end_of_final_command = final_command;
    size_t command_size = 0;
    ssize_t current_input_size = 0, input_size = 0;
    bool is_last_backshlash = true;
    while(is_last_backshlash) {
        current_input_size = getline(&input_line, &command_size, stdin);
        if(current_input_size < 1) {
            return NULL;
        }
        input_line[current_input_size - 1] = '\0';
        current_input_size--;
        is_last_backshlash = (current_input_size > 0 && input_line[current_input_size - 1] == '\\');

        input_size += current_input_size;
        if(size_of_final_command < input_size + 1) {
            int new_size = size_of_final_command;
            while(new_size < input_size + 1)    new_size <<= 1;
            char* new_final_command = (char *) malloc(new_size * sizeof(char));
            if(new_final_command == NULL) {
                write(STDERR_FILENO, malloc_error_message, strlen(malloc_error_message));
                return NULL;
            }

            char* t1 = final_command;
            char* t2 = input_line;
            char* t3 = new_final_command;
            while(*t1 != '\0') {
                *t3 = *t1;
                t1++;
                t3++;
            }
            while(*t2 != '\0') {
                *t3 = *t2;
                t2++;
                t3++;
            }
            *t3 = '\0';
            free(final_command);
            final_command = new_final_command;
            size_of_final_command = new_size;
            end_of_final_command = t3;
        }
        else {
            char* t1 = input_line;
            while(*t1 != '\0') {
                *end_of_final_command = *t1;
                t1++;
                end_of_final_command++;
            }
            *end_of_final_command = '\0';
        }

        free(input_line);
        input_line = NULL;
        command_size = 0;
    }
    *final_input_size = input_size;
    return final_command;
}

#endif