#pragma once

#include "lexer.h"
#define ARGUMENTS_CONST 256

typedef enum {
    CMD_EXEC,
    CMD_PIPE,
    CMD_REDIRECT
} CmdType;

struct Command {
    CmdType type;
};

struct ExecCmd {
    CmdType type;
    char* argv[ARGUMENTS_CONST]; // must be null terminated for execvp
};

struct PipeCmd {
    CmdType type;
    struct Command* left;
    struct Command* right;
};

struct RedirectCmd {
    CmdType type;
    struct Command* cmd;
    char* file;
    int mode;
    int fd;
};

struct Command* parse(TokenArray* tokenarray);
void free_ast(struct Command* cmd);