#include "tokeniser.h"
#include "utils.h"

void tokenise_string(JSONTokeniser *t) {
    assert(*t->current_char == '\"' ||
           *t->current_char == '\'' && "String must start with a quote");

    int max = 100;
    int i = 0;
    char *literal = calloc(1, sizeof(char) * max);

    char quote = *t->current_char;
    (t->current_char)++;
    while (*t->current_char != quote) {
        literal[i] = *t->current_char;
        i += 1;
        (t->current_char)++;

        if (i == max - 1) {
            max *= 2;
            char *new_literal = calloc(1, sizeof(char) * max);
            memcpy(new_literal, literal, sizeof(char) * i);
            free(literal);
            literal = new_literal;
        }
    }
    t->current_char++;

    t->current_token.type = STRING;
    t->current_token.value.string = literal;
}

void tokenise_true(JSONTokeniser *t) {
    if (strncmp(t->current_char, "true", 4) == 0) {
        t->current_token.type = TRUE;
        t->current_char += 4;
        return;
    }
    fprintf(stderr, "Invalid token: %s\n", t->current_char);
    exit(1);
}

void tokenise_false(JSONTokeniser *t) {
    if (strncmp(t->current_char, "false", 5) == 0) {
        t->current_token.type = FALSE;
        t->current_char += 5;
        return;
    }
    fprintf(stderr, "Invalid token: %s\n", t->current_char);
    exit(1);
}

void tokenise_null(JSONTokeniser *t) {
    if (strncmp(t->current_char, "null", 4) == 0) {
        t->current_token.type = NULL_TOKEN;
        t->current_char += 4;
        return;
    }
    fprintf(stderr, "Invalid token: %s\n", t->current_char);
    exit(1);
}

void tokenise_number(JSONTokeniser *t) {
    int max = 100;
    int i = 0;
    char *literal = calloc(1, sizeof(char) * max);

    while (is_digit(*t->current_char)) {
        literal[i] = *t->current_char;
        i += 1;
        (t->current_char)++;

        if (i == max - 1) {
            max *= 2;
            char *new_literal = calloc(1, sizeof(char) * max);
            memcpy(new_literal, literal, sizeof(char) * i);
            free(literal);
            literal = new_literal;
        }
    }

    if (*t->current_char != '.') {
        t->current_token.type = NUMBER_INT;
        t->current_token.value.number_int = atol(literal);
        free(literal);
        return;
    }

    literal[i] = '.';
    i += 1;
    (t->current_char)++;

    while (is_digit(*t->current_char)) {
        literal[i] = *t->current_char;
        i += 1;
        (t->current_char)++;

        if (i == max - 1) {
            max *= 2;
            char *new_literal = calloc(1, sizeof(char) * max);
            memcpy(new_literal, literal, sizeof(char) * i);
            free(literal);
            literal = new_literal;
        }
    }

    t->current_token.type = NUMBER_FLOAT;
    t->current_token.value.number_float = atof(literal);
    free(literal);
}

JSONTokeniser *tokenise(char *content) {
    size_t max = 100;

    JSONTokeniser *t = calloc(1, sizeof(JSONTokeniser));
    *t = (JSONTokeniser){
        .tokens = calloc(1, sizeof(JSONToken) * max),
        .token_count = 0,
        .content = content,
        .current_char = content,
        .current_token = {0},
    };

    while (*t->current_char != '\0') {
        if (is_whitespace(*t->current_char)) {
            t->current_char++;
            continue;
        }

        switch (*t->current_char) {
        case '{':
            t->current_token = (JSONToken){.type = LEFT_CURLY};
            t->current_char++;
            break;
        case '}':
            t->current_token = (JSONToken){.type = RIGHT_CURLY};
            t->current_char++;
            break;
        case '[':
            t->current_token = (JSONToken){.type = LEFT_SQUARE};
            t->current_char++;
            break;
        case ']':
            t->current_token = (JSONToken){.type = RIGHT_SQUARE};
            t->current_char++;
            break;
        case ',':
            t->current_token = (JSONToken){.type = COMMA};
            t->current_char++;
            break;
        case ':':
            t->current_token = (JSONToken){.type = COLON};
            t->current_char++;
            break;
        case '"':
        case '\'':
            tokenise_string(t);
            break;
        case '0' ... '9':
            tokenise_number(t);
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
        if (t->token_count == max - 2) {
            max *= 2;
            JSONToken *new_tokens = calloc(1, sizeof(JSONToken) * max);
            memcpy(new_tokens, t->tokens, sizeof(JSONToken) * t->token_count);
            free(t->tokens);
            t->tokens = new_tokens;
        }

        // Add token to array
        t->tokens[t->token_count] = t->current_token;
        t->token_count++;
        t->current_token = (JSONToken){0};
    }
    t->tokens[t->token_count] = (JSONToken){.type = END};
    return t;
}

void free_tokeniser(JSONTokeniser *t) {
    for (size_t i = 0; i < t->token_count; ++i) {
        if (t->tokens[i].type == STRING) {
            free(t->tokens[i].value.string);
        }
    }
    free(t->tokens);
    free(t);
}
