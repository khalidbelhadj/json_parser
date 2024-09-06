#pragma once

#include "tokeniser.h"

// ----------
// JSON Value
// ----------

typedef enum {
    JSON_VALUE_STRING,
    JSON_VALUE_NUMBER_INT,
    JSON_VALUE_NUMBER_FLOAT,
    JSON_VALUE_BOOLEAN,
    JSON_VALUE_NULL,
} JSONValuetype;

static const char *value_type_names[] = {
    [0] = "string",
    [1] = "int",
    [2] = "float",
    [3] = "boolean",
    [4] = "null"
};

typedef struct {
    JSONValuetype type;
    union {
        char *string;
        long long number_int;
        long double number_float;
        bool boolean;
    } value;
} JSONValue;

// ------------
// JSON Element
// ------------

typedef enum {
    JSON_ELEMENT_OBJECT,
    JSON_ELEMENT_ARRAY,
    JSON_ELEMENT_VALUE,
    JSON_ELEMENT_END
} JSONElementType;

static const char *element_type_names[] = {
    [0] = "object",
    [1] = "array",
    [2] = "value",
    [3] = "end of file"
};

struct JSONObject;
struct JSONArray;

typedef struct {
    JSONElementType type;
    union {
        struct JSONObject *object;
        struct JSONArray *array;
        JSONValue value;
    } element;
} JSONElement;

// ----------
// JSON Array
// ----------

typedef struct JSONArray {
    JSONElement *elements;
    size_t elements_count;
} JSONArray;

// -----------
// JSON Object
// -----------

typedef struct JSONPair {
    char *key;
    JSONElement value;
} JSONPair;

typedef struct JSONObject {
    JSONPair *pairs;
    size_t pairs_count;
} JSONObject;

// -----------
// JSON Parser
// -----------

typedef struct {
    JSONElement root;
    JSONToken *tokens;
    size_t token_count;
    size_t current_token;
    const char *file_name;
} JSONParser;

JSONElement parse(Arena *a, char *content, int *error);
JSONElement parse_file(Arena *a, const char *file_name, int *error);
JSONElement parse_element(Arena *a, JSONParser *p, int *error);
JSONArray *parse_array(Arena *a, JSONParser *p, int *error);
JSONObject *parse_object(Arena *a, JSONParser *p, int *error);
JSONElement parse_string(JSONParser *p, int *error);
JSONElement parse_number(JSONParser *p, int *error);
JSONElement parse_true(JSONParser *p, int *error);
JSONElement parse_false(JSONParser *p, int *error);
JSONElement parse_null(JSONParser *p, int *error);

char *stringify(Arena *a, JSONElement element);

void print_element(JSONElement *element, int indent);
