#pragma once

#include <stddef.h>

typedef enum {
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_REDIRECT_OUT,
    TOKEN_REDIRECT_IN,
    TOKEN_BACKGROUND,
    TOKEN_EOF,
} TokenType;

typedef struct {
    TokenType type;
    char* value;
} Token;

typedef struct {
    Token* tokens;
    int count;
    int capacity;
} TokenArray;

TokenArray* tokenize(const char* input);
void free_token_array(TokenArray* array);
