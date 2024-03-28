#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIST_OF_TOKENS(X)                                                      \
    X(LEFT_CURLY)                                                              \
    X(RIGHT_CURLY)                                                             \
    X(LEFT_SQUARE)                                                             \
    X(RIGHT_SQUARE)                                                            \
    X(COMMA)                                                                   \
    X(COLON)                                                                   \
    X(NUMBER_INT)                                                              \
    X(NUMBER_FLOAT)                                                            \
    X(STRING)                                                                  \
    X(TRUE)                                                                    \
    X(FALSE)                                                                   \
    X(NULL_TOKEN)                                                              \
    X(END)

#define TOKEN(token) token,
typedef enum { LIST_OF_TOKENS(TOKEN) } JSONTokenType;
#undef TOKEN

#define TOKEN(token) #token,
static const char *token_names[] = {LIST_OF_TOKENS(TOKEN)};

typedef struct {
    JSONTokenType type;
    union {
        char *string;
        long long number_int;
        long double number_float;
        bool boolean;
    } value;
} JSONToken;

typedef struct {
    JSONToken *tokens;
    size_t token_count;
    char *content;
    char *current_char;
    JSONToken current_token;
} JSONTokeniser;

void tokenise_string(JSONTokeniser *t);
void tokenise_true(JSONTokeniser *t);
void tokenise_false(JSONTokeniser *t);
void tokenise_null(JSONTokeniser *t);
JSONTokeniser *tokenise(char *content);
void free_tokeniser(JSONTokeniser *t);