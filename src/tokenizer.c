#include "tokenizer.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

// Max string length 1MB
#define MAX_STRING_LENGTH (1 << 20)

JSONTokenizer *json_tokenize(Arena *a, const char *content, int *error) {
    *error = 0;

    if (!content) {
        *error = 1;
        return NULL;
    }

    JSONTokenizer *t = arena_alloc(a, sizeof(JSONTokenizer));
    if (!t) {
        *error = 1;
        return NULL;
    }

    // Add +1 to ensure space for null terminator
    size_t content_len = strlen(content) + 1;
    char *content_copy = arena_alloc(a, sizeof(char) * content_len);
    if (!content_copy) {
        *error = 1;
        return NULL;
    }

    strncpy(content_copy, content, content_len - 1);
    content_copy[content_len - 1] = '\0';  // Ensure null termination

    *t = (JSONTokenizer){.content = content_copy,
                         .current_char = content_copy,
                         .current_token = {0},
                         .current_line = 1,
                         .current_col = 1,
                         .tokens = NULL,
                         .token_count = 0};

    Arena tmp = {0};
    size_t capacity = INIT_CAPACITY;
    size_t size = 0;
    JSONToken *tokens = arena_alloc(&tmp, sizeof(JSONToken) * INIT_CAPACITY);
    if (!tokens) {
        *error = 1;
        arena_free(&tmp);
        return t;
    }

    while (*t->current_char != '\0' && !(*error)) {
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
                if (json_tokenize_string(a, t)) {
                    fprintf(stderr, "Line %zu, Col %zu: Invalid string\n",
                            t->current_line, t->current_col);
                    *error = 1;
                }
                break;
            case '-':
            case '0' ... '9':
                if (json_tokenize_number(a, t)) {
                    fprintf(stderr, "Line %zu, Col %zu: Invalid number\n",
                            t->current_line, t->current_col);
                    *error = 1;
                }
                break;
            case 't':
                if (json_tokenize_true(t)) {
                    fprintf(
                        stderr,
                        "Line %zu, Col %zu: Invalid token starting with 't'\n",
                        t->current_line, t->current_col);
                    *error = 1;
                }
                break;
            case 'f':
                if (json_tokenize_false(t)) {
                    fprintf(
                        stderr,
                        "Line %zu, Col %zu: Invalid token starting with 'f'\n",
                        t->current_line, t->current_col);
                    *error = 1;
                }
                break;
            case 'n':
                if (json_tokenize_null(t)) {
                    fprintf(
                        stderr,
                        "Line %zu, Col %zu: Invalid token starting with 'n'\n",
                        t->current_line, t->current_col);
                    *error = 1;
                }
                break;
            default:
                fprintf(stderr, "Line %zu, Col %zu: Unknown character: %c\n",
                        t->current_line, t->current_col, *t->current_char);
                *error = 1;
        }

        if (*error) break;

        // Resize array if needed
        if (size + 1 >= capacity) {
            capacity *= 2;
            JSONToken *new_tokens =
                arena_alloc(&tmp, sizeof(JSONToken) * capacity);
            if (!new_tokens) {
                *error = 1;
                arena_free(&tmp);
                return t;
            }
            memcpy(new_tokens, tokens, sizeof(JSONToken) * size);
            tokens = new_tokens;
        }

        // Add token to array
        tokens[size] = t->current_token;
        ++size;
        t->current_token = (JSONToken){0};
    }

    if (!(*error)) {
        tokens[size] = (JSONToken){.type = END};
        ++size;

        t->token_count = size;
        t->tokens = arena_alloc(a, sizeof(JSONToken) * size);
        if (!t->tokens) {
            *error = 1;
            arena_free(&tmp);
            return t;
        }
        memcpy(t->tokens, tokens, sizeof(JSONToken) * size);
    } else {
        // Set empty token array on error
        t->token_count = 0;
        t->tokens = NULL;
    }

    arena_free(&tmp);
    return t;
}

int json_tokenize_string(Arena *a, JSONTokenizer *t) {
    if (!t || !t->current_char || !(*t->current_char)) {
        return 1;  // Error
    }

    char quote = *t->current_char;
    if (quote != '"' && quote != '\'') {
        return 1;  // Error - not a string
    }

    t->current_token.line = t->current_line;
    t->current_token.col = t->current_col;

    // Move past opening quote
    ++t->current_char;
    ++t->current_col;

    // Find the length of the string and check for closing quote
    size_t literal_len = 0;
    while (t->current_char[literal_len] != '\0') {
        if (t->current_char[literal_len] == quote) {
            break;
        }

        // Handle escaped characters
        if (t->current_char[literal_len] == '\\' &&
            t->current_char[literal_len + 1] != '\0') {
            literal_len += 2;  // Skip the escape sequence
        } else {
            ++literal_len;
        }

        // Check for max string length
        if (literal_len > MAX_STRING_LENGTH) {
            fprintf(stderr,
                    "Line %zu, Col %zu: String exceeds maximum length\n",
                    t->current_line, t->current_col);
            return 1;  // Error
        }
    }

    // Check if we reached end of input without closing quote
    if (t->current_char[literal_len] == '\0') {
        fprintf(stderr, "Line %zu, Col %zu: Unterminated string\n",
                t->current_line, t->current_col);
        return 1;  // Error
    }

    // Allocate space for the string and handle escape sequences
    char *literal = arena_alloc(a, sizeof(char) * (literal_len + 1));
    if (!literal) {
        return 1;  // Memory allocation error
    }

    size_t j = 0;
    for (size_t i = 0; i < literal_len; i++) {
        if (t->current_char[i] == '\\' && i + 1 < literal_len) {
            // Handle escape sequences
            i++;  // Skip the backslash
            switch (t->current_char[i]) {
                case '\\':
                case '\'':
                case '"':
                    literal[j++] = t->current_char[i];
                    break;
                case 'n':
                    literal[j++] = '\n';
                    break;
                case 't':
                    literal[j++] = '\t';
                    break;
                case 'r':
                    literal[j++] = '\r';
                    break;
                default:
                    // Just include both characters for other sequences
                    literal[j++] = '\\';
                    literal[j++] = t->current_char[i];
            }
        } else {
            literal[j++] = t->current_char[i];
        }
    }

    literal[j] = '\0';  // Null terminate the string

    t->current_token.value.string = literal;
    t->current_token.type = STRING;

    // Move past the closing quote
    t->current_char += literal_len + 1;
    t->current_col += literal_len + 1;

    return 0;  // Success
}

int json_tokenize_true(JSONTokenizer *t) {
    if (!t || !t->current_char) {
        return 1;  // Error
    }

    if (strncmp(t->current_char, "true", 4) == 0) {
        t->current_token.type = TRUE;
        t->current_token.line = t->current_line;
        t->current_token.col = t->current_col;
        t->current_char += 4;
        t->current_col += 4;
        return 0;  // Success
    }
    return 1;  // Error
}

int json_tokenize_false(JSONTokenizer *t) {
    if (!t || !t->current_char) {
        return 1;  // Error
    }

    if (strncmp(t->current_char, "false", 5) == 0) {
        t->current_token.type = FALSE;
        t->current_token.line = t->current_line;
        t->current_token.col = t->current_col;
        t->current_char += 5;
        t->current_col += 5;
        return 0;  // Success
    }
    return 1;  // Error
}

int json_tokenize_null(JSONTokenizer *t) {
    if (!t || !t->current_char) {
        return 1;  // Error
    }

    if (strncmp(t->current_char, "null", 4) == 0) {
        t->current_token.type = NULL_TOKEN;
        t->current_token.line = t->current_line;
        t->current_token.col = t->current_col;
        t->current_char += 4;
        t->current_col += 4;
        return 0;  // Success
    }
    return 1;  // Error
}

int json_tokenize_number(Arena *a, JSONTokenizer *t) {
    if (!t || !t->current_char) {
        return 1;  // Error
    }

    t->current_token.line = t->current_line;
    t->current_token.col = t->current_col;

    // Check for negative sign
    bool negative = false;
    size_t literal_len = 0;

    if (t->current_char[literal_len] == '-') {
        negative = true;
        ++literal_len;
    }

    // Count digits before decimal point
    while (t->current_char[literal_len] != '\0' &&
           is_digit(t->current_char[literal_len])) {
        ++literal_len;
    }

    // Check if we have a decimal point
    bool is_float = false;
    if (t->current_char[literal_len] == '.') {
        is_float = true;
        ++literal_len;

        // Count digits after decimal point
        while (t->current_char[literal_len] != '\0' &&
               is_digit(t->current_char[literal_len])) {
            ++literal_len;
        }
    }

    // Check for at least one digit
    if ((literal_len == 0) || (literal_len == 1 && negative)) {
        return 1;  // Error - not a number
    }

    // Allocate memory for the number string
    char *literal = arena_alloc(a, sizeof(char) * (literal_len + 1));
    if (!literal) {
        return 1;  // Memory allocation error
    }

    memcpy(literal, t->current_char, literal_len);
    literal[literal_len] = '\0';

    t->current_char += literal_len;
    t->current_col += literal_len;

    if (is_float) {
        // Convert to float and check errors
        char *endptr;
        errno = 0;
        long double value = strtold(literal, &endptr);

        if (errno == ERANGE) {
            fprintf(stderr, "Line %zu, Col %zu: Float number out of range\n",
                    t->current_token.line, t->current_token.col);
            return 1;  // Error
        }

        if (*endptr != '\0') {
            fprintf(stderr, "Line %zu, Col %zu: Invalid float format\n",
                    t->current_token.line, t->current_token.col);
            return 1;  // Error
        }

        t->current_token.type = NUMBER_FLOAT;
        t->current_token.value.number_float = value;
    } else {
        // Convert to integer and check errors
        char *endptr;
        errno = 0;
        long long value = strtoll(literal, &endptr, 10);

        if (errno == ERANGE) {
            fprintf(stderr, "Line %zu, Col %zu: Integer number out of range\n",
                    t->current_token.line, t->current_token.col);
            return 1;  // Error
        }

        if (*endptr != '\0') {
            fprintf(stderr, "Line %zu, Col %zu: Invalid integer format\n",
                    t->current_token.line, t->current_token.col);
            return 1;  // Error
        }

        t->current_token.type = NUMBER_INT;
        t->current_token.value.number_int = value;
    }

    return 0;  // Success
}