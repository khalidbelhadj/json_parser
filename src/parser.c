#include "../include/parser.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#define log_error(p, tok, expected)                          \
    fprintf(stderr, "%s:%zu:%zu: Expected %s, got %s\n", p->file_name, tok.line,                   \
            tok.col, token_names[expected], token_names[tok.type])

JSONElement parse_file(Arena *a, const char *file_name, int *error) {
    char *content = read_file_content(file_name);
    if (content == NULL) {
        printf("Failed to read file %s\n", file_name);
        *error = 1;
        return (JSONElement){0};
    }

    JSONTokeniser *t = tokenise(a, content);
    JSONParser *p = arena_alloc(a, sizeof(JSONParser));
    *p = (JSONParser){.tokens = t->tokens,
                      .token_count = t->token_count,
                      .current_token = 0,
                      .file_name = realpath(file_name, NULL)};

    p->root = parse_element(a, p, error);
    return p->root;
}

JSONElement parse(Arena *a, char *content, int *error) {
    JSONTokeniser *t = tokenise(a, content);
    JSONParser *p = arena_alloc(a, sizeof(JSONParser));
    *p = (JSONParser){
        .tokens = t->tokens,
        .token_count = t->token_count,
        .current_token = 0,
    };

    p->root = parse_element(a, p, error);
    return p->root;
}

JSONElement parse_element(Arena *a, JSONParser *p, int *error) {
    JSONElement element = {0};
    JSONToken tok = p->tokens[p->current_token];
    switch (tok.type) {
    case LEFT_CURLY:
        element.type = JSON_ELEMENT_OBJECT;
        element.element.object = parse_object(a, p, error);
        return element;
    case LEFT_SQUARE:
        element.type = JSON_ELEMENT_ARRAY;
        element.element.array = parse_array(a, p, error);
        return element;
    case STRING:
        return parse_string(p, error);
    case NUMBER_INT:
    case NUMBER_FLOAT:
        return parse_number(p, error);
    case TRUE:
        return parse_true(p, error);
    case FALSE:
        return parse_false(p, error);
    case NULL_TOKEN:
        return parse_null(p, error);
    default:
        log_error(p, tok, NULL_TOKEN);
        *error = 1;
        return element;
    }
}

JSONArray *parse_array(Arena *a, JSONParser *p, int *error) {
    JSONArray *array = arena_alloc(a, sizeof(JSONArray));

    Arena tmp = {0};
    size_t capacity = INIT_CAPACITY;
    size_t size = 0;
    JSONElement *elements = arena_alloc(&tmp, sizeof(JSONElement) * capacity);

    // Parse opening square brace
    JSONToken opening = p->tokens[p->current_token];
    ++p->current_token;
    if (opening.type != LEFT_SQUARE) {
        log_error(p, opening, LEFT_SQUARE);
        *error = 1;
        return array;
    }

    while (true) {
        JSONElement value = parse_element(a, p, error);
        if (*error != 0)
            return array;

        if (size + 1 >= capacity) {
            capacity *= 2;
            JSONElement *new_elements =
                arena_alloc(&tmp, sizeof(JSONElement) * capacity);
            memcpy(new_elements, elements, sizeof(JSONElement) * size);
            elements = new_elements;
        }

        elements[size] = value;
        ++size;

        JSONToken comma = p->tokens[p->current_token];
        ++p->current_token;

        if (comma.type != COMMA && comma.type != RIGHT_SQUARE) {
            fprintf(stderr, "%s:%zu:%zu: Expected %s or %s, got %s\n",
                    p->file_name, comma.line, comma.col, token_names[COMMA],
                    token_names[RIGHT_SQUARE], token_names[comma.type]);
            *error = 1;
            return array;
        }

        if (comma.type == RIGHT_SQUARE)
            break;
    }

    array->elements_count = size;
    array->elements = arena_alloc(a, sizeof(JSONElement) * size);
    memcpy(array->elements, elements, sizeof(JSONElement) * size);

    arena_free(&tmp);
    return array;
}

JSONObject *parse_object(Arena *a, JSONParser *p, int *error) {
    JSONObject *object = arena_alloc(a, sizeof(JSONObject));

    Arena tmp = {0};
    size_t capacity = INIT_CAPACITY;
    size_t size = 0;
    JSONPair *pairs = arena_alloc(&tmp, sizeof(JSONPair) * capacity);

    // Parse opening left curly
    JSONToken opening = p->tokens[p->current_token];
    ++p->current_token;
    if (opening.type != LEFT_CURLY) {
        fprintf(stderr, "Expected %s, got %s\n", token_names[LEFT_CURLY],
                token_names[opening.type]);
        *error = 1;
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
        JSONElement value = parse_element(a, p, error);
        if (*error != 0)
            return object;
        JSONPair pair = {.key = key.value.string, .value = value};

        if (size + 1 >= capacity) {
            capacity *= 2;
            JSONPair *new_pairs =
                arena_alloc(&tmp, sizeof(JSONPair) * capacity);
            memcpy(new_pairs, pairs, sizeof(JSONPair) * size);
            pairs = new_pairs;
        }

        pairs[size] = pair;
        ++size;

        // Parse comma
        JSONToken comma = p->tokens[p->current_token];
        ++p->current_token;

        if (comma.type != COMMA && comma.type != RIGHT_CURLY) {
            fprintf(stderr, "Expected %s or %s, got %s\n", token_names[COMMA],
                    token_names[RIGHT_CURLY], token_names[comma.type]);
            *error = 1;
            return object;
        }

        if (comma.type == RIGHT_CURLY)
            break;
    }

    object->pairs_count = size;
    object->pairs = arena_alloc(a, sizeof(JSONPair) * size);
    memcpy(object->pairs, pairs, sizeof(JSONPair) * size);

    arena_free(&tmp);
    return object;
}

JSONElement parse_string(JSONParser *p, int *error) {
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

JSONElement parse_number(JSONParser *p, int *error) {
    JSONToken number_token = p->tokens[p->current_token];
    JSONElement element = {.type = JSON_ELEMENT_VALUE};

    ++p->current_token;

    switch (number_token.type) {
    case NUMBER_INT:
        element.element.value.type = JSON_VALUE_NUMBER_INT;
        element.element.value.value.number_int = number_token.value.number_int;
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

JSONElement parse_true(JSONParser *p, int *error) {
    JSONToken token = p->tokens[p->current_token];
    JSONElement element = {0};
    ++p->current_token;
    if (token.type != TRUE) {
        fprintf(stderr, "Expected true, got %s\n", token_names[token.type]);
        *error = 1;
        return element;
    }
    element.type = JSON_ELEMENT_VALUE;
    element.element.value.type = JSON_VALUE_BOOLEAN;
    element.element.value.value.boolean = true;
    return element;
}

JSONElement parse_false(JSONParser *p, int *error) {
    JSONToken token = p->tokens[p->current_token];
    JSONElement element = {0};
    ++p->current_token;
    if (token.type != FALSE) {
        fprintf(stderr, "Expected false, got %s\n", token_names[token.type]);
        *error = 1;
        return element;
    }
    element.type = JSON_ELEMENT_VALUE;
    element.element.value.type = JSON_VALUE_BOOLEAN;
    element.element.value.value.boolean = false;
    return element;
}

JSONElement parse_null(JSONParser *p, int *error) {
    JSONToken token = p->tokens[p->current_token];
    JSONElement element = {0};
    ++p->current_token;
    if (token.type != NULL_TOKEN) {
        fprintf(stderr, "Expected null, got %s\n", token_names[token.type]);
        *error = 0;
        return element;
    }
    element.type = JSON_ELEMENT_VALUE;
    element.element.value.type = JSON_VALUE_NULL;
    return element;
}

char *stringify(Arena *a, JSONElement element) {
    // TODO: Use arenas here rather than calloc
    switch (element.type) {
    case JSON_ELEMENT_OBJECT: {
        size_t length = 1;
        for (size_t i = 0; i < element.element.object->pairs_count; ++i) {
            length += strlen(element.element.object->pairs[i].key) + 4;
            length +=
                strlen(stringify(a, element.element.object->pairs[i].value));
            if (i != element.element.object->pairs_count - 1) {
                length += 2;
            }
        }
        char *buffer = calloc(1, sizeof(char) * length);
        strcat(buffer, "{");
        for (size_t i = 0; i < element.element.object->pairs_count; ++i) {
            strcat(buffer, "\"");
            strcat(buffer, element.element.object->pairs[i].key);
            strcat(buffer, "\"");
            strcat(buffer, ": ");
            strcat(buffer,
                   stringify(a, element.element.object->pairs[i].value));
            if (i != element.element.object->pairs_count - 1) {
                strcat(buffer, ", ");
            }
        }
        strcat(buffer, "}");
        return buffer;
    }
    case JSON_ELEMENT_ARRAY: {
        size_t length = 1;
        for (size_t i = 0; i < element.element.array->elements_count; ++i) {
            length += strlen(stringify(a, element.element.array->elements[i]));
            if (i != element.element.array->elements_count - 1) {
                length += 2;
            }
        }
        char *buffer = calloc(1, sizeof(char) * length);
        strcat(buffer, "[");
        for (size_t i = 0; i < element.element.array->elements_count; ++i) {
            strcat(buffer, stringify(a, element.element.array->elements[i]));
            if (i != element.element.array->elements_count - 1) {
                strcat(buffer, ", ");
            }
        }
        strcat(buffer, "]");
        return buffer;
    }
    case JSON_ELEMENT_VALUE:
        switch (element.element.value.type) {
        case JSON_VALUE_STRING: {
            size_t length = strlen(element.element.value.value.string) + 3;
            char *buffer = calloc(1, sizeof(char) * length);
            snprintf(buffer, length, "\"%s\"",
                     element.element.value.value.string);
            return buffer;
        }
        case JSON_VALUE_NUMBER_INT: {
            char *buffer = calloc(1, sizeof(char) * 101);
            snprintf(buffer, 100, "%llu",
                     element.element.value.value.number_int);
            return strdup(buffer);
        }
        case JSON_VALUE_NUMBER_FLOAT: {
            char *buffer = calloc(1, sizeof(char) * 100);
            snprintf(buffer, 100, "%Lf",
                     element.element.value.value.number_float);
            return strdup(buffer);
        }
        case JSON_VALUE_BOOLEAN:
            if (element.element.value.value.boolean) {
                return "true";
            }
            return "false";
        case JSON_VALUE_NULL:
            return "null";
        }
        break;
    case JSON_ELEMENT_END:
        return "";
    }
    return NULL;
}

void print_element(JSONElement *element, int indent) {
    switch (element->type) {
    case JSON_ELEMENT_VALUE:
        switch (element->element.value.type) {
        case JSON_VALUE_STRING:
            printf("%*c\"%s\"\n", indent, ' ',
                   element->element.value.value.string);
            break;
        case JSON_VALUE_NUMBER_INT:
            printf("%*c%llu\n", indent, ' ',
                   element->element.value.value.number_int);
            break;
        case JSON_VALUE_NUMBER_FLOAT:
            printf("%*c%Lf\n", indent, ' ',
                   element->element.value.value.number_float);
            break;
        case JSON_VALUE_BOOLEAN:
            printf("%*c%s\n", indent, ' ',
                   element->element.value.value.boolean ? "true" : "false");
            break;
        case JSON_VALUE_NULL:
            printf("%*cnull\n", indent, ' ');
            break;
        }
        break;
    case JSON_ELEMENT_ARRAY:
        printf("%*c[\n", indent, ' ');
        for (size_t i = 0; i < element->element.array->elements_count; ++i) {
            print_element(&element->element.array->elements[i], indent + 4);
        }
        printf("%*c[\n", indent, ' ');
        break;
    case JSON_ELEMENT_OBJECT:
        for (size_t i = 0; i < element->element.object->pairs_count; ++i) {
            printf("%*c%s:\n", indent, ' ',
                   element->element.object->pairs[i].key);
            print_element(&element->element.object->pairs[i].value, indent + 4);
        }
        break;
    case JSON_ELEMENT_END:
        printf("End of JSON\n");
        break;
    }
}
