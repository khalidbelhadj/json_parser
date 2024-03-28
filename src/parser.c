#include "parser.h"
#include "tokeniser.h"

JSONElement parse_string(JSONParser *p) {
    JSONToken token = p->tokens[p->current_token];
    p->current_token++;
    if (token.type != STRING) {
        fprintf(stderr, "Expected string, got %s\n", token_names[token.type]);
        exit(1);
    }
    JSONElement element;
    element.type = JSON_ELEMENT_VALUE;
    element.element.value.type = JSON_VALUE_STRING;
    element.element.value.value.string = token.value.string;
    return element;
}

JSONElement parse_number(JSONParser *p) {
    JSONToken token = p->tokens[p->current_token];
    p->current_token++;
    JSONElement element;
    element.type = JSON_ELEMENT_VALUE;
    switch (token.type) {
    case NUMBER_INT:
        element.element.value.type = JSON_VALUE_NUMBER_INT;
        element.element.value.value.number_int = token.value.number_int;
        break;
    case NUMBER_FLOAT:
        element.element.value.type = JSON_VALUE_NUMBER_FLOAT;
        element.element.value.value.number_float = token.value.number_float;
        break;
    default:
        fprintf(stderr, "Expected number, got %s\n", token_names[token.type]);
        exit(1);
    }

    return element;
}

JSONElement parse_true(JSONParser *p) {
    JSONToken token = p->tokens[p->current_token];
    p->current_token++;
    if (token.type != TRUE) {
        fprintf(stderr, "Expected true, got %s\n", token_names[token.type]);
        exit(1);
    }
    JSONElement element;
    element.type = JSON_ELEMENT_VALUE;
    element.element.value.type = JSON_VALUE_BOOLEAN;
    element.element.value.value.boolean = true;
    return element;
}

JSONElement parse_false(JSONParser *p) {
    JSONToken token = p->tokens[p->current_token];
    p->current_token++;
    if (token.type != FALSE) {
        fprintf(stderr, "Expected false, got %s\n", token_names[token.type]);
        exit(1);
    }
    JSONElement element;
    element.type = JSON_ELEMENT_VALUE;
    element.element.value.type = JSON_VALUE_BOOLEAN;
    element.element.value.value.boolean = false;
    return element;
}

JSONElement parse_null(JSONParser *p) {
    JSONToken token = p->tokens[p->current_token];
    p->current_token++;
    if (token.type != NULL_TOKEN) {
        fprintf(stderr, "Expected null, got %s\n", token_names[token.type]);
        exit(1);
    }
    JSONElement element;
    element.type = JSON_ELEMENT_VALUE;
    element.element.value.type = JSON_VALUE_NULL;
    return element;
}

JSONElement parse_element(JSONParser *p);

JSONObject *parse_object(JSONParser *p) {
    size_t max = 100;
    JSONObject *object = calloc(1, sizeof(JSONObject));
    *object = (JSONObject){
        .pairs = calloc(1, sizeof(JSONPair) * max),
        .pair_count = 0,
    };

    JSONToken opening = p->tokens[p->current_token];
    p->current_token++;
    if (opening.type != LEFT_CURLY) {
        fprintf(stderr, "Expected {, got %s\n", token_names[opening.type]);
        exit(1);
    }

    while (p->tokens[p->current_token].type != RIGHT_CURLY) {
        JSONToken key = p->tokens[p->current_token];
        if (key.type != STRING) {
            fprintf(stderr, "Expected string, got %s, line: %d, token number: %zu\n", token_names[key.type], __LINE__, p->current_token);
            exit(1);
        }
        p->current_token++;

        JSONToken colon = p->tokens[p->current_token];
        p->current_token++;
        if (colon.type != COLON) {
            fprintf(stderr, "Expected :, got %s\n", token_names[colon.type]);
            exit(1);
        }

        JSONElement value = parse_element(p);
        JSONPair pair = {
            .key = key.value.string,
            .value = value,
        };

        if (object->pair_count == max - 2) {
            max *= 2;
            JSONPair *new_pairs = calloc(1, sizeof(JSONPair) * max);
            memcpy(new_pairs, object->pairs,
                   sizeof(JSONPair) * object->pair_count);
            free(object->pairs);
            object->pairs = new_pairs;
        }

        object->pairs[object->pair_count] = pair;
        object->pair_count++;

        JSONToken comma = p->tokens[p->current_token];
        p->current_token++;
        if (comma.type != COMMA && comma.type != RIGHT_CURLY) {
            fprintf(stderr, "Expected comma, got %s\n",
                    token_names[comma.type]);
            exit(1);
        }

        if (comma.type == RIGHT_CURLY) {
            break;
        }
    }
    return object;
}

JSONArray *parse_array(JSONParser *p) {
    size_t max = 100;
    JSONArray *array = calloc(1, sizeof(JSONArray));
    *array = (JSONArray){
        .elements = calloc(1, sizeof(JSONValue) * max),
        .element_count = 0,
    };

    JSONToken opening = p->tokens[p->current_token];
    p->current_token++;
    if (opening.type != LEFT_SQUARE) {
        fprintf(stderr, "Expected [, got %s\n", token_names[opening.type]);
        exit(1);
    }

    while (p->tokens[p->current_token].type != RIGHT_SQUARE) {
        JSONElement value = parse_element(p);

        JSONToken comma = p->tokens[p->current_token];
        if (comma.type != COMMA && comma.type != RIGHT_SQUARE) {
            fprintf(stderr, "Expected comma, got %s\n",
                    token_names[comma.type]);
            exit(1);
        }

        if (array->element_count == max - 2) {
            max *= 2;
            JSONElement *new_values = calloc(1, sizeof(JSONElement) * max);
            memcpy(new_values, array->elements,
                   sizeof(JSONElement) * array->element_count);
            free(array->elements);
            array->elements = new_values;
        }
        array->elements[array->element_count] = value;
        array->element_count++;
        p->current_token++;

        if (comma.type == RIGHT_SQUARE) {
            break;
        }
    }
    return array;
}

JSONElement parse_element(JSONParser *p) {
    JSONElement element;
    JSONToken token = p->tokens[p->current_token];
    switch (token.type) {
    case LEFT_CURLY:
        element.type = JSON_ELEMENT_OBJECT;
        element.element.object = parse_object(p);
        return element;
    case LEFT_SQUARE:
        element.type = JSON_ELEMENT_ARRAY;
        element.element.array = parse_array(p);
        return element;
    case STRING:
        return parse_string(p);
    case NUMBER_INT:
    case NUMBER_FLOAT:
        return parse_number(p);
    case TRUE:
        return parse_true(p);
    case FALSE:
        return parse_false(p);
    case NULL_TOKEN:
        return parse_null(p);
    case END:
        element.type = JSON_ELEMENT_END;
        return element;
    default:
        fprintf(stderr, "Expected element, got %s\n", token_names[token.type]);
        exit(1);
        break;
    }
}

JSONParser *parse(JSONTokeniser *t) {
    JSONParser *p = calloc(1, sizeof(JSONParser));
    *p = (JSONParser){
        .tokens = t->tokens,
        .token_count = t->token_count,
        .current_token = 0,
    };

    p->root = parse_element(p);
    return p;
}

void free_parser(JSONParser *p) {
    free(p->tokens);
    free(p);
}

void print_element(JSONElement element, int indent) {
    switch (element.type) {
    case JSON_ELEMENT_VALUE:
        switch (element.element.value.type) {
        case JSON_VALUE_STRING:
            printf("%*c\"%s\"\n", indent, ' ',
                   element.element.value.value.string);
            break;
        case JSON_VALUE_NUMBER_INT:
            printf("%*c%llu\n", indent, ' ', element.element.value.value.number_int);
            break;
        case JSON_VALUE_NUMBER_FLOAT:
            printf("%*c%Lf\n", indent, ' ', element.element.value.value.number_float);
            break;
        case JSON_VALUE_BOOLEAN:
            printf("%*c%s\n", indent, ' ',
                   element.element.value.value.boolean ? "true" : "false");
            break;
        case JSON_VALUE_NULL:
            printf("%*cnull\n", indent, ' ');
            break;
        }
        break;
    case JSON_ELEMENT_ARRAY:
        for (size_t i = 0; i < element.element.array->element_count; ++i) {
            print_element(element.element.array->elements[i], indent);
        }
        break;
    case JSON_ELEMENT_OBJECT:
        for (size_t i = 0; i < element.element.object->pair_count; ++i) {
            printf("%*c%s:\n", indent, ' ',
                   element.element.object->pairs[i].key);
            print_element(element.element.object->pairs[i].value, indent + 4);
        }
        break;
    case JSON_ELEMENT_END:
        printf("End of JSON\n");
        break;
    }
}
