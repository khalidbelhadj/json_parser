#include "../include/tokeniser.h"

#include "../include/utils.h"
#include <string.h>
#include <sys/_types/_null.h>

JSONTokeniser *tokenise(Arena *a, const char *content) {
    JSONTokeniser *t = arena_alloc(a, sizeof(JSONTokeniser));
    char *content_copy = arena_alloc(a, sizeof(char) * (strlen(content) + 1));
    strncpy(content_copy, content, strlen(content));

    *t = (JSONTokeniser) {
        .content = content_copy, .current_char = content_copy,
        .current_token = {0}, .current_line = 1, .current_col = 1
    };

    Arena tmp = {0};
    size_t capacity = INIT_CAPACITY;
    size_t size = 0;
    JSONToken *tokens = arena_alloc(&tmp, sizeof(JSONToken) * INIT_CAPACITY);

    while (*t->current_char != '\0') {
        switch (*t->current_char) {
        case ' ':
        case '\t':
        case '\r':
            t->current_token.line = t->current_line;
            t->current_token.col = t->current_col;
            ++t->current_col;
            ++t->current_char;
            continue;
        case '\n':
            ++t->current_line;
            t->current_col = 1;
            ++t->current_char;
            continue;
        case '{':
            t->current_token = (JSONToken){.type = LEFT_CURLY};
            t->current_token.line = t->current_line;
            t->current_token.col = t->current_col;
            ++t->current_col;
            ++t->current_char;
            break;
        case '}':
            t->current_token = (JSONToken){.type = RIGHT_CURLY};
            t->current_token.line = t->current_line;
            t->current_token.col = t->current_col;
            ++t->current_col;
            ++t->current_char;
            break;
        case '[':
            t->current_token = (JSONToken){.type = LEFT_SQUARE};
            t->current_token.line = t->current_line;
            t->current_token.col = t->current_col;
            ++t->current_col;
            ++t->current_char;
            break;
        case ']':
            t->current_token = (JSONToken){.type = RIGHT_SQUARE};
            t->current_token.line = t->current_line;
            t->current_token.col = t->current_col;
            ++t->current_col;
            ++t->current_char;
            break;
        case ',':
            t->current_token = (JSONToken){.type = COMMA};
            t->current_token.line = t->current_line;
            t->current_token.col = t->current_col;
            ++t->current_col;
            ++t->current_char;
            break;
        case ':':
            t->current_token = (JSONToken){.type = COLON};
            t->current_token.line = t->current_line;
            t->current_token.col = t->current_col;
            ++t->current_col;
            ++t->current_char;
            break;
        case '"':
        case '\'':
            tokenise_string(a, t);
            break;
        case '0' ... '9':
            tokenise_number(a, t);
            break;
        case 't':
            tokenise_true(t);
            break;
        case 'f':
            tokenise_false(t);
            break;
        case 'n':
            tokenise_null(t);
            break;
        default:
            fprintf(stderr, "Unknown character: %c\n", *t->current_char);
            exit(1);
        }

        // Resize array if needed
        if (size + 1 >= capacity) {
            capacity *= 2;
            JSONToken *new_tokens =
                arena_alloc(&tmp, sizeof(JSONToken) * capacity);
            memcpy(new_tokens, tokens, sizeof(JSONToken) * size);
            tokens = new_tokens;
        }

        // Add token to array
        tokens[size] = t->current_token;
        ++size;
        t->current_token = (JSONToken){0};
    }
    tokens[size] = (JSONToken){.type = END};
    ++size;

    t->token_count = size;
    t->tokens = arena_alloc(a, sizeof(JSONToken) * size);
    memcpy(t->tokens, tokens, sizeof(JSONToken) * size);

    arena_free(&tmp);
    return t;
}

void tokenise_string(Arena *a, JSONTokeniser *t) {
    assert(*t->current_char == '\"' ||
           *t->current_char == '\'' && "String must start with a quote");

    char quote = *t->current_char;
    ++t->current_char;

    size_t literal_len = 0;
    while (t->current_char[literal_len] != quote)
        ++literal_len;

    char *literal = arena_alloc(a, sizeof(char) * (literal_len + 1));
    memcpy(literal, t->current_char, literal_len);

    t->current_token.value.string = literal;
    t->current_char += literal_len + 1;
    t->current_token.type = STRING;
    t->current_col += literal_len + 2;
}

void tokenise_true(JSONTokeniser *t) {
    if (strncmp(t->current_char, "true", 4) == 0) {
        t->current_token.type = TRUE;
        t->current_char += 4;
        t->current_col += 4;
        return;
    }

    fprintf(stderr, "Invalid token: %s\n", t->current_char);
    exit(1);
}

void tokenise_false(JSONTokeniser *t) {
    if (strncmp(t->current_char, "false", 5) == 0) {
        t->current_token.type = FALSE;
        t->current_char += 5;
        t->current_col += 5;
        return;
    }

    fprintf(stderr, "Invalid token: %s\n", t->current_char);
    exit(1);
}

void tokenise_null(JSONTokeniser *t) {
    if (strncmp(t->current_char, "null", 4) == 0) {
        t->current_token.type = NULL_TOKEN;
        t->current_char += 4;
        t->current_col += 4;
        return;
    }
    fprintf(stderr, "Invalid token: %s\n", t->current_char);
    exit(1);
}

void tokenise_number(Arena *a, JSONTokeniser *t) {

    size_t literal_len = 0;
    while (is_digit(t->current_char[literal_len]))
        ++literal_len;

    if (t->current_char[literal_len] != '.') {
        char *literal = arena_alloc(a, sizeof(char) * (literal_len + 1));
        memcpy(literal, t->current_char, literal_len);

        t->current_char += literal_len;
        t->current_token.type = NUMBER_INT;
        t->current_token.value.number_int = atol(literal);
        t->current_col += literal_len;
        return;
    }

    ++literal_len;
    while (is_digit(t->current_char[literal_len]))
        ++literal_len;

    char *literal = arena_alloc(a, sizeof(char) * (literal_len + 1));
    memcpy(literal, t->current_char, literal_len);

    t->current_char += literal_len;
    t->current_token.type = NUMBER_FLOAT;
    t->current_token.value.number_float = atof(literal);
    t->current_col += literal_len;
}