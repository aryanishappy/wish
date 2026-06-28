#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

struct Command* build_exec_cmd() {
    struct ExecCmd* cmd = malloc(sizeof(struct ExecCmd));
    cmd->type = CMD_EXEC;
    for(int i = 0; i < ARGUMENTS_CONST; i++)    cmd->argv[i] = NULL;
    return (struct Command *) cmd;
}

struct Command* build_pipe_cmd(struct Command* left, struct Command* right) {
    struct PipeCmd* cmd = malloc(sizeof(struct PipeCmd));
    cmd->type = CMD_PIPE;
    cmd->left = left;
    cmd->right = right;
    return (struct Command *) cmd;
}

struct Command* build_redirect_cmd(struct Command* cmd_to_redirect, char* file, int mode, int fd) {
    struct RedirectCmd* cmd = malloc(sizeof(struct RedirectCmd));
    cmd->type = CMD_REDIRECT;
    cmd->cmd = cmd_to_redirect;
    cmd->file = file;
    cmd->mode = mode;
    cmd->fd = fd;
    return (struct Command *) cmd;
}

void free_ast(struct Command* cmd) {
    if(cmd == NULL) return;
    else if(cmd->type == CMD_EXEC) {
        free(cmd);
    }
    else if(cmd->type == CMD_PIPE) {
        struct PipeCmd* pcmd = (struct PipeCmd *) cmd;
        free_ast(pcmd->left);
        free_ast(pcmd->right);
        free(pcmd);
    }
    else if(cmd->type == CMD_REDIRECT) {
        struct RedirectCmd* rcmd = (struct RedirectCmd *) cmd;
        free_ast(rcmd->cmd);
        free(rcmd);
    }
}

struct Command* parse_exec(TokenArray* ta, int* pos) {
    struct Command* cmd = build_exec_cmd();
    struct ExecCmd* ecmd = (struct ExecCmd *) cmd;
    int argc = 0;

    while(ta->tokens[*pos].type != TOKEN_EOF && ta->tokens[*pos].type != TOKEN_PIPE) {
        if(ta->tokens[*pos].type == TOKEN_WORD) {
            if(argc == ARGUMENTS_CONST - 1) {
                fprintf(stderr, "Exceeeded the max arguments count\n");
                free_ast(cmd);
                return NULL;
            }
            ecmd->argv[argc++] = ta->tokens[*pos].value;
            (*pos)++; 
        }
        else if(ta->tokens[*pos].type == TOKEN_REDIRECT_IN) {
            (*pos)++;

            if(ta->tokens[*pos].type != TOKEN_WORD) {
                fprintf(stderr, "Syntax error near Redirect in token\n");
                free_ast(cmd);
                return NULL;
            }

            char* filename = ta->tokens[*pos].value;
            (*pos)++;
            cmd = build_redirect_cmd(cmd, filename, O_RDONLY, 0);
        }
        else if(ta->tokens[*pos].type == TOKEN_REDIRECT_OUT) {
            (*pos)++;

            if(ta->tokens[*pos].type != TOKEN_WORD) {
                fprintf(stderr, "Syntax error near Redirect out token\n");
                free_ast(cmd);
                return NULL;
            }

            char* filename = ta->tokens[*pos].value;
            (*pos)++;
            cmd = build_redirect_cmd(cmd, filename, O_WRONLY | O_CREAT | O_TRUNC, 1);
        }
        else {
            fprintf(stderr, "Unexpected token\n");
            free_ast(cmd);
            return NULL;
        }
    }
    return cmd;
}

struct Command* parse_pipeline(TokenArray* ta, int* pos) {
    struct Command* cmd = parse_exec(ta, pos);
    if (cmd == NULL) return NULL;
    if(ta->tokens[*pos].type == TOKEN_PIPE) {
        (*pos)++;
        struct Command* right_cmd = parse_pipeline(ta, pos);
        if (right_cmd == NULL) {
            free_ast(cmd);
            return NULL;
        }

        cmd = build_pipe_cmd(cmd, right_cmd);
    }
    return cmd;
}

struct Command* parse(TokenArray* tokenarray) {
    int pos = 0;
    struct Command* root = parse_pipeline(tokenarray, &pos);
    return root;
}

