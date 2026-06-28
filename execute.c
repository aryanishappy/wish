#include "parser.h"
#include "path.h"
#include "execute.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>

void execute_ast(struct Command* cmd) {
    if(cmd == NULL) exit(1);

    if(cmd->type == CMD_EXEC) {
        struct ExecCmd* ecmd = (struct ExecCmd *) cmd;

        if(ecmd->argv[0] == NULL)   exit(1);
        
        struct path* head = malloc(sizeof(struct path));
        head->path = strdup("./");
        head->next = paths;
        bool can_access = false;
        char* executable_path = NULL;
        while(head != NULL) {
            char* absolute_command = (char *) malloc((strlen(ecmd->argv[0]) + strlen(head->path) + 2) * sizeof(char));
            if(absolute_command == NULL) {
                head = head->next;
                continue;
            }
            int t = strlen(head->path);
            for(int i = 0; i < t; i++) absolute_command[i] = (head->path)[i];
            absolute_command[t++] = '/';
            for(int i = 0; i < strlen(ecmd->argv[0]); i++)    absolute_command[t++] = ecmd->argv[0][i];
            absolute_command[t] = '\0';

            if(access(absolute_command, X_OK) == 0) {
                can_access = true;
                executable_path = absolute_command;
                break;
            }
            free(absolute_command);
            head = head->next;
        }

        if(can_access) {
            execv(executable_path, ecmd->argv);

            free(executable_path);
            fprintf(stderr, "execv failed\n");
            exit(1);
        }
        else {
            fprintf(stderr, "Command not found\n");
            exit(1);
        }
    }
    else if(cmd->type == CMD_REDIRECT) {
        struct RedirectCmd* rcmd = (struct RedirectCmd *) cmd;
        int file_fd = open(rcmd->file, rcmd->mode, 0644);

        if(file_fd < 0) {
            fprintf(stderr, "Could not open file %s\n", rcmd->file);
            exit(1);
        }

        dup2(file_fd, rcmd->fd);
        close(file_fd);

        execute_ast(rcmd->cmd);
    }
    else if(cmd->type == CMD_PIPE) {
        struct PipeCmd* pcmd = (struct PipeCmd *) cmd;
        int p[2];
        if(pipe(p) < 0) {
            fprintf(stderr, "Could not set up pipe\n");
            exit(1);
        }

        // left
        int lt = fork();
        if(lt < 0) {
            fprintf(stderr, "Fork failed\n");
            exit(1);
        }
        if(lt == 0) {
            close(p[0]);
            dup2(p[1], 1);
            close(p[1]); // already duplicated
            execute_ast(pcmd->left);
        }

        // right
        int rt = fork();
        if(rt < 0) {
            fprintf(stderr, "Fork failed\n");
            exit(1);
        }
        if(rt == 0) {
            close(p[1]);
            dup2(p[0], 0);
            close(p[0]);
            execute_ast(pcmd->right);
        }

        close(p[0]);
        close(p[1]);

        waitpid(lt, NULL, 0);
        waitpid(rt, NULL, 0);
        exit(0);
    }
}

int execute_builtin(struct Command* cmd) {
    if(cmd->type != CMD_EXEC)   return 0;
    struct ExecCmd* ecmd = (struct ExecCmd *) cmd;
    
    if(strcmp(ecmd->argv[0], "exit") == 0) {
        if(ecmd->argv[1] == NULL) {
            exit(0);
        }
        else {
            fprintf(stderr, "Invalid command. Possible use: exit\n");
        }
        return 1;
    }
    else if(strcmp(ecmd->argv[0], "cd") == 0) {
        if(ecmd->argv[1] == NULL || ecmd->argv[2] != NULL) {
            fprintf(stderr, "Invalid use of command. Possible use: cd <dir>\n");
        }
        else if(chdir(ecmd->argv[1]) != 0){
            fprintf(stderr, "Could not change current directory to %s", ecmd->argv[1]);
        }
        return 1;
    }
    else if(strcmp(ecmd->argv[0], "path") == 0) {
        while(paths != NULL) {
            struct path* temp = paths;
            paths = paths->next;
            free(temp->path);
            free(temp);
        }
        int ind = 1;
        while(ecmd->argv[ind] != NULL)  ind++;
        while(--ind != 0) {
            struct path* new_path = (struct path *) malloc(sizeof(struct path));
            new_path->path = strdup(ecmd->argv[ind]);
            new_path->next = paths;
            paths = new_path;
        }
        return 1;
    }
    return 0;
}

void run_command(struct Command* cmd) {
    if(cmd == NULL) return;

    if(execute_builtin(cmd) == 1)   return;
    int ch = fork();
    if(ch < 0) {
        fprintf(stderr, "Fork failed\n");
    }
    else if(ch == 0) {
        execute_ast(cmd);
        exit(0);
    }
    else {
        wait(NULL);
    }
}