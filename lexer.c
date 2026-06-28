#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 1024

typedef enum {
    STATE_NORMAL,
    STATE_IN_WORD,
    STATE_IN_QUOTES,
    STATE_TAKE_LITERALLY
} LexerState;

void update_array(TokenArray* tokenarray, TokenType tokentype, char* token) {
    if(tokenarray->count == tokenarray->capacity) {
        tokenarray->capacity <<= 1;
        Token* new_array = realloc(tokenarray->tokens, tokenarray->capacity * sizeof(Token));
        if(new_array == NULL) exit(1);
        tokenarray->tokens = new_array;
    }
    tokenarray->tokens[tokenarray->count].type = tokentype;
    tokenarray->tokens[tokenarray->count].value = token;
    tokenarray->count++;
} 

TokenArray* tokenize(const char* input) {
    TokenArray* tokenarray = (TokenArray *) malloc(sizeof(TokenArray));
    tokenarray->capacity = 16;
    tokenarray->count = 0;
    tokenarray->tokens = malloc(tokenarray->capacity * sizeof(Token));

    char buffer[BUFFER_SIZE], c;
    const char* temp = input;
    int buff_index = 0;
    LexerState state = STATE_NORMAL;
    while((c = *temp) != '\0') {
        switch(state) {
            case STATE_NORMAL:
                if(c == ' ' || c == '\t') {
                }
                else if(c == '|') {
                    update_array(tokenarray, TOKEN_PIPE, strdup("|"));
                    buff_index = 0;
                }
                else if(c == '>') {
                    update_array(tokenarray, TOKEN_REDIRECT_OUT, strdup(">"));
                    buff_index = 0;
                }
                else if(c == '<') {
                    update_array(tokenarray, TOKEN_REDIRECT_IN, strdup("<"));
                    buff_index = 0;
                }
                else if(c == '"') {
                    state = STATE_IN_QUOTES;
                }
                else if(c == '\\') {
                    state = STATE_TAKE_LITERALLY;
                }
                else {
                    state = STATE_IN_WORD;
                    buffer[buff_index++] = c;
                }
                break;
            
            case STATE_IN_WORD:
                if(buff_index == BUFFER_SIZE) exit(1);
                if(c == ' ' || c == '\t') {
                    buffer[buff_index] = '\0';
                    update_array(tokenarray, TOKEN_WORD, strdup(buffer));
                    buff_index = 0;
                    state = STATE_NORMAL;
                }
                else if(c == '|' || c == '>' || c == '<') {
                    buffer[buff_index] = '\0';
                    update_array(tokenarray, TOKEN_WORD, strdup(buffer));
                    temp--;
                    buff_index = 0;
                    state = STATE_NORMAL;
                }
                else if(c == '\\') {
                    state = STATE_TAKE_LITERALLY;
                }
                else {
                    buffer[buff_index++] = c;
                }
                break;

            case STATE_TAKE_LITERALLY:
                if(buff_index == BUFFER_SIZE)   exit(1);
                buffer[buff_index++] = c;
                state = STATE_IN_WORD;
                break;

            case STATE_IN_QUOTES:
                if(buff_index == BUFFER_SIZE)   exit(1);
                if(c == '"') {
                    buffer[buff_index] = '\0';
                    update_array(tokenarray, TOKEN_WORD, strdup(buffer));
                    buff_index = 0;
                    state = STATE_NORMAL;
                }
                else {
                    buffer[buff_index++] = c;
                }
                break;
        }
        temp++;
    }
    if(buff_index > 0) {
        buffer[buff_index] = '\0';
        update_array(tokenarray, TOKEN_WORD, strdup(buffer));
    }
    update_array(tokenarray, TOKEN_EOF, NULL);

    return tokenarray;
}

void free_token_array(TokenArray* tokenarray) {
    for(int i = 0; i < tokenarray->count; i++)  {
        free((tokenarray->tokens)[i].value);
    }
    free(tokenarray->tokens);
    free(tokenarray);
}