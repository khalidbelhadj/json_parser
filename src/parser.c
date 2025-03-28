#include "../include/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/syslimits.h>

#include "../include/utils.h"
#include "arena.h"
#include "tokenizer.h"

static void json_error_token(JSONParser *p, size_t line, size_t col,
                             const char *expected, const char *got) {
    const char *source = p->file_name ? p->file_name : "<string>";

    fprintf(stderr, "%s:%zu:%zu: Expected %s, got %s\n", source, line, col,
            expected, got);
}

static void json_error(JSONParser *p, const char *message) {
    const char *source = p->file_name ? p->file_name : "<string>";

    fprintf(stderr, "%s: %s\n", source, message);
}

JSONElement json_parse_file(Arena *a, const char *file_name, int *error) {
    char *path = realpath(file_name, NULL);
    if (path == NULL) {
        printf("Failed to read file %s\n", file_name);
        *error = 1;
        return (JSONElement){0};
    }

    char *content = read_file_content(path);
    if (content == NULL) {
        printf("Failed to read file %s\n", file_name);
        *error = 1;
        free(path);
        return (JSONElement){0};
    }

    JSONTokenizer *t = json_tokenize(a, content, error);
    JSONParser *p = arena_alloc(a, sizeof(JSONParser));

    *p = (JSONParser){.tokens = t->tokens,
                      .token_count = t->token_count,
                      .current_token = 0,
                      .file_name = path};

    p->root = json_parse_element(a, p, error);

    free(content);
    free(path);
    return p->root;
}

JSONElement json_parse(Arena *a, char *content, int *error) {
    JSONTokenizer *t = json_tokenize(a, content, error);
    JSONParser *p = arena_alloc(a, sizeof(JSONParser));
    *p = (JSONParser){
        .tokens = t->tokens,
        .token_count = t->token_count,
        .current_token = 0,
    };

    p->root = json_parse_element(a, p, error);
    return p->root;
}

JSONElement json_parse_element(Arena *a, JSONParser *p, int *error) {
    JSONElement element = {0};
    JSONToken tok = p->tokens[p->current_token];

    if (p->current_depth >= JSON_MAX_DEPTH) {
        json_error(p, "Maximum JSON element depth reached");
        *error = 1;
        return element;
    }

    ++p->current_depth;

    switch (tok.type) {
        case LEFT_CURLY:
            element.type = JSON_ELEMENT_OBJECT;
            element.element.object = json_parse_object(a, p, error);
            break;
        case LEFT_SQUARE:
            element.type = JSON_ELEMENT_ARRAY;
            element.element.array = json_parse_array(a, p, error);
            break;
        case STRING:
            element = json_parse_string(p, error);
            break;
        case NUMBER_INT:
        case NUMBER_FLOAT:
            element = json_parse_number(p, error);
            break;
        case TRUE:
            element.type = JSON_ELEMENT_VALUE;
            element.element.value.type = JSON_VALUE_BOOLEAN;
            element.element.value.value.boolean = true;
            p->current_token++;
            break;
        case FALSE:
            element.type = JSON_ELEMENT_VALUE;
            element.element.value.type = JSON_VALUE_BOOLEAN;
            element.element.value.value.boolean = false;
            p->current_token++;
            break;
        case NULL_TOKEN:
            element.type = JSON_ELEMENT_VALUE;
            element.element.value.type = JSON_VALUE_NULL;
            p->current_token++;
            break;
        default:
            if (p->file_name) {
                fprintf(stderr, "%s:%zu:%zu: Expected %s, got %s\n",
                        p->file_name, tok.line, tok.col, "json element",
                        token_names[tok.type]);
            } else {
                fprintf(stderr, "%zu:%zu: Expected %s, got %s\n", tok.line,
                        tok.col, "json element", token_names[tok.type]);
            }
            *error = 1;
    }

    --p->current_depth;
    return element;
}

JSONArray *json_parse_array(Arena *a, JSONParser *p, int *error) {
    JSONArray *array = arena_alloc(a, sizeof(JSONArray));
    *array = (JSONArray){.head = NULL, .tail = NULL};

    // Parse opening square brace
    JSONToken opening = p->tokens[p->current_token];
    ++p->current_token;
    if (opening.type != LEFT_SQUARE) {
        if (p->file_name) {
            fprintf(stderr, "%s:%zu:%zu: Expected %s, got %s\n", p->file_name,
                    opening.line, opening.col, token_names[LEFT_SQUARE],
                    token_names[opening.type]);
        } else {
            fprintf(stderr, "%zu:%zu: Expected %s, got %s\n", opening.line,
                    opening.col, token_names[LEFT_SQUARE],
                    token_names[opening.type]);
        }
        *error = 1;
        return array;
    }

    // Try parsing closing right square
    // This is a special case for empty arrays
    JSONToken closing = p->tokens[p->current_token];
    if (closing.type == RIGHT_SQUARE) {
        ++p->current_token;
        return array;
    }

    while (true) {
        JSONElement element = json_parse_element(a, p, error);
        if (*error != 0) {
            return array;
        }

        JSONArrayElement *array_element =
            arena_alloc(a, sizeof(JSONArrayElement));

        *array_element = (JSONArrayElement){.element = element, .next = NULL};

        if (array->head == NULL) {
            array->head = array_element;
            array->tail = array_element;
        } else {
            array->tail->next = array_element;
            array->tail = array_element;
        }

        JSONToken comma = p->tokens[p->current_token];
        ++p->current_token;

        if (comma.type != COMMA && comma.type != RIGHT_SQUARE) {
            fprintf(stderr, "%s:%zu:%zu: Expected %s or %s, got %s\n",
                    p->file_name, comma.line, comma.col, token_names[COMMA],
                    token_names[RIGHT_SQUARE], token_names[comma.type]);
            *error = 1;
            return array;
        }

        if (comma.type == RIGHT_SQUARE) break;
    }

    return array;
}

JSONObject *json_parse_object(Arena *a, JSONParser *p, int *error) {
    JSONObject *object = arena_alloc(a, sizeof(JSONObject));
    *object = (JSONObject){.head = NULL, .tail = NULL};

    // Parse opening left curly
    JSONToken opening = p->tokens[p->current_token];
    ++p->current_token;
    if (opening.type != LEFT_CURLY) {
        fprintf(stderr, "Expected %s, got %s\n", token_names[LEFT_CURLY],
                token_names[opening.type]);
        *error = 1;
        return object;
    }

    // Try to parse the closing right curly
    // This is a special case for empty objects
    JSONToken closing = p->tokens[p->current_token];
    if (closing.type == RIGHT_CURLY) {
        ++p->current_token;
        return object;
    }

    while (true) {
        // Parse key
        JSONToken key = p->tokens[p->current_token];
        ++p->current_token;
        if (key.type != STRING) {
            fprintf(stderr, "Expected %s, got %s\n", token_names[STRING],
                    token_names[key.type]);
            *error = 1;
            return object;
        }

        // Parse colon
        JSONToken colon = p->tokens[p->current_token];
        ++p->current_token;
        if (colon.type != COLON) {
            fprintf(stderr, "Expected %s, got %s\n", token_names[COLON],
                    token_names[colon.type]);
            *error = 1;
            return object;
        }

        // Parse value
        JSONElement value = json_parse_element(a, p, error);
        if (*error != 0) {
            return object;
        }

        // Add pair to object linked list
        JSONPair *pair = arena_alloc(a, sizeof(JSONPair));
        *pair = (JSONPair){.key = key.value.string, .value = value};

        if (object->head == NULL) {
            object->head = pair;
            object->tail = pair;
        } else {
            object->tail->next = pair;
            object->tail = pair;
        }
        object->tail->next = NULL;

        // Parse comma, or closing curly
        JSONToken comma = p->tokens[p->current_token];
        ++p->current_token;

        if (comma.type == RIGHT_CURLY) break;

        if (comma.type != COMMA) {
            fprintf(stderr, "Expected %s or %s, got %s\n", token_names[COMMA],
                    token_names[RIGHT_CURLY], token_names[comma.type]);
            *error = 1;
            return object;
        }
    }

    return object;
}

JSONElement json_parse_string(JSONParser *p, int *error) {
    JSONElement element = {0};
    JSONToken string_token = p->tokens[p->current_token];
    ++p->current_token;
    if (string_token.type != STRING) {
        fprintf(stderr, "Expected string, got %s\n",
                token_names[string_token.type]);
        *error = 1;
        return element;
    }
    element.type = JSON_ELEMENT_VALUE;
    element.element.value.type = JSON_VALUE_STRING;
    element.element.value.value.string = string_token.value.string;
    return element;
}

JSONElement json_parse_number(JSONParser *p, int *error) {
    JSONToken number_token = p->tokens[p->current_token];
    JSONElement element = {.type = JSON_ELEMENT_VALUE};

    ++p->current_token;

    switch (number_token.type) {
        case NUMBER_INT:
            element.element.value.type = JSON_VALUE_NUMBER_INT;
            element.element.value.value.number_int =
                number_token.value.number_int;
            break;
        case NUMBER_FLOAT:
            element.element.value.type = JSON_VALUE_NUMBER_FLOAT;
            element.element.value.value.number_float =
                number_token.value.number_float;
            break;
        default:
            fprintf(stderr, "Expected number, got %s\n",
                    token_names[number_token.type]);
            *error = 1;
            return element;
    }

    return element;
}

char *json_stringify(Arena *a, JSONElement element) {
    switch (element.type) {
        case JSON_ELEMENT_OBJECT: {
            // Calculate required buffer size
            size_t length = 2;  // For {} characters
            JSONPair *current = element.element.object->head;
            bool first = true;

            // Calculate length needed for the JSON string
            while (current != NULL) {
                if (!first) {
                    length += 2;  // For ", " between pairs
                }
                // Key length + 2 for quotes + 2 for ": "
                length += strlen(current->key) + 4;
                // Value length
                char *value_str = json_stringify(a, current->value);
                length += strlen(value_str);

                first = false;
                current = current->next;
            }

            // Allocate memory from arena
            char *buffer =
                arena_alloc(a, length + 1);  // +1 for null terminator

            // Build JSON object string
            buffer[0] = '{';
            buffer[1] = '\0';

            current = element.element.object->head;
            first = true;

            while (current != NULL) {
                if (!first) {
                    strcat(buffer, ", ");
                }

                // Add key with quotes
                char *key_buf = arena_alloc(a, strlen(current->key) + 3);
                snprintf(key_buf, strlen(current->key) + 3, "\"%s\"",
                         current->key);
                strcat(buffer, key_buf);

                // Add colon separator
                strcat(buffer, ": ");

                // Add value
                char *value_str = json_stringify(a, current->value);
                strcat(buffer, value_str);

                first = false;
                current = current->next;
            }

            strcat(buffer, "}");
            return buffer;
        }
        case JSON_ELEMENT_ARRAY: {
            // Use the linked list structure instead of array access
            size_t length = 2;  // For [] characters
            JSONArrayElement *current = element.element.array->head;
            bool first = true;

            // Calculate length needed for the JSON string
            while (current != NULL) {
                if (!first) {
                    length += 2;  // For ", " between elements
                }
                // Value length
                char *elem_str = json_stringify(a, current->element);
                length += strlen(elem_str);

                first = false;
                current = current->next;
            }

            // Allocate memory from arena
            char *buffer =
                arena_alloc(a, length + 1);  // +1 for null terminator

            // Build JSON array string
            buffer[0] = '[';
            buffer[1] = '\0';

            current = element.element.array->head;
            first = true;

            while (current != NULL) {
                if (!first) {
                    strcat(buffer, ", ");
                }

                // Add value
                char *elem_str = json_stringify(a, current->element);
                strcat(buffer, elem_str);

                first = false;
                current = current->next;
            }

            strcat(buffer, "]");
            return buffer;
        }
        case JSON_ELEMENT_VALUE:
            switch (element.element.value.type) {
                case JSON_VALUE_STRING: {
                    size_t length = strlen(element.element.value.value.string) +
                                    3;  // +2 for quotes, +1 for null
                    char *buffer = arena_alloc(a, length);
                    snprintf(buffer, length, "\"%s\"",
                             element.element.value.value.string);
                    return buffer;
                }
                case JSON_VALUE_NUMBER_INT: {
                    char *buffer = arena_alloc(a, 32);  // Enough for int64
                    snprintf(buffer, 32, "%lld",
                             element.element.value.value.number_int);
                    return buffer;
                }
                case JSON_VALUE_NUMBER_FLOAT: {
                    char *buffer =
                        arena_alloc(a, 64);  // Enough for long double
                    snprintf(buffer, 64, "%Lf",
                             element.element.value.value.number_float);
                    return buffer;
                }
                case JSON_VALUE_BOOLEAN: {
                    char *result = element.element.value.value.boolean
                                       ? arena_alloc_str(a, "true")
                                       : arena_alloc_str(a, "false");
                    return result;
                }
                case JSON_VALUE_NULL: {
                    return arena_alloc_str(a, "null");
                }
            }
            break;
        case JSON_ELEMENT_END:
            return arena_alloc_str(a, "");
    }
    return NULL;
}
