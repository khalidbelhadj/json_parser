#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arena.h"

#define INIT_CAPACITY (1 << 10)

typedef enum {
    LEFT_CURLY,
    RIGHT_CURLY,
    LEFT_SQUARE,
    RIGHT_SQUARE,
    COMMA,
    COLON,
    NUMBER_INT,
    NUMBER_FLOAT,
    STRING,
    TRUE,
    FALSE,
    NULL_TOKEN,
    END,
} JSONTokenType;

static const char *token_names[] = {
    [0] = "{",      [1] = "}",     [2] = "[",     [3] = "]",      [4] = ",",
    [5] = ":",      [6] = "int",   [7] = "float", [8] = "string", [9] = "true",
    [10] = "false", [11] = "null", [12] = "eof",
};

typedef struct {
    JSONTokenType type;
    union {
        char *string;
        long long number_int;
        long double number_float;
        bool boolean;
    } value;
    size_t line;
    size_t col;
} JSONToken;

typedef struct {
    size_t current_line;
    size_t current_col;
    JSONToken *tokens;
    size_t token_count;
    const char *content;
    char *current_char;
    JSONToken current_token;
} JSONTokenizer;

JSONTokenizer *json_tokenize(Arena *a, const char *content, int *error);
int json_tokenize_string(Arena *a, JSONTokenizer *t);
int json_tokenize_true(JSONTokenizer *t);
int json_tokenize_false(JSONTokenizer *t);
int json_tokenize_null(JSONTokenizer *t);
int json_tokenize_number(Arena *a, JSONTokenizer *t);
