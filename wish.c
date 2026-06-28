#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include "getting_input.h"
#include "input_processing.h"
#include "error_messages.h"
#include "lexer.h"
#include "path.h"
#include "parser.h"
#include "execute.h"
// write(STDERR_FILENO, error_message, strlen(error_message)); 

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

    path_initialization();
    // Infinite loop until user enters the command 'exit'
    while(1) {
        //Prompt user for input
        if(flag) {
            char current_dir[1000];
            if(getcwd(current_dir, 1000) == NULL) {
                fprintf(stderr, "Cannot access current directory name\n");
                exit(1);
            }
            printf("wish: [%s] ", current_dir);
        }

        // Get the whole input in full_command
        ssize_t input_size = 0;
        char* full_command = get_input(&input_size);
        if(full_command == NULL) {
            fprintf(stderr, "Input processing failed. Please retry with proper input.\n");
            continue;
        }
        char** commands = break_command(full_command);
        char** current_command = commands;
        while(*current_command != NULL) {
            TokenArray* tokens = tokenize(*current_command);
            struct Command* ast = parse(tokens);

            if(ast != NULL) {
                run_command(ast);
            }

            free_ast(ast);
            free_token_array(tokens);
            free(*current_command);
            current_command++;
        }
        free(commands);
        free(full_command);
    }

    free_path();
    return 0;
}