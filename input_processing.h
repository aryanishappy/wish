#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char** break_command(char* full_command) {
    char *c = full_command, *st = full_command;
    char** commands = (char **) malloc(100 * sizeof(char *));
    for(int i = 0; i < 100; i++)    commands[i] = NULL;
    int index = 0, currsz = 0;
    char prev = 'k';
    while(*c && *(c + 1)) {
        if(prev != '\\' && *c == '&' && *(c + 1) == '&') {
            if(index == 100) {
                fprintf(stderr, "Number of processes > 100\n");
                exit(1);
            }
            if(currsz > 0) commands[index++] = strndup(st, currsz);
            currsz = 0;
            prev = 'k';
            st = (c += 2);
        }
        else {
            currsz++;
            prev = *c;
            c++;
        }
    }
    if(index == 100) {
        fprintf(stderr, "Number of processes > 100\n");
        exit(1);
    }
    commands[index] = strndup(st, currsz + 1);

    return commands;
}

