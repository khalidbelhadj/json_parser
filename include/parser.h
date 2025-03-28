#pragma once

#include "tokenizer.h"

#define JSON_MAX_DEPTH 1024
#define JSON_MAX_ELEMENTS 1000000

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
    [0] = "string", [1] = "int", [2] = "float", [3] = "boolean", [4] = "null"};

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
    [0] = "object", [1] = "array", [2] = "value", [3] = "end of file"};

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

#define for_each_element(arr, elem_var)                              \
    for (JSONArrayElement *elem_var = (arr)->head; elem_var != NULL; \
         elem_var = elem_var->next)

typedef struct JSONArrayElement {
    JSONElement element;
    struct JSONArrayElement *next;
} JSONArrayElement;

typedef struct JSONArray {
    JSONArrayElement *head;
    JSONArrayElement *tail;
} JSONArray;

// -----------
// JSON Object
// -----------

#define for_each_pair(obj, pair_var)                         \
    for (JSONPair *pair_var = (obj)->head; pair_var != NULL; \
         pair_var = pair_var->next)

typedef struct JSONPair {
    char *key;
    JSONElement value;
    struct JSONPair *next;
} JSONPair;

typedef struct JSONObject {
    JSONPair *head;
    JSONPair *tail;
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
    size_t current_depth;
} JSONParser;

JSONElement json_parse(Arena *a, char *content, int *error);
JSONElement json_parse_file(Arena *a, const char *file_name, int *error);
JSONElement json_parse_element(Arena *a, JSONParser *p, int *error);
JSONArray *json_parse_array(Arena *a, JSONParser *p, int *error);
JSONObject *json_parse_object(Arena *a, JSONParser *p, int *error);
JSONElement json_parse_string(JSONParser *p, int *error);
JSONElement json_parse_number(JSONParser *p, int *error);

char *json_stringify(Arena *a, JSONElement element);
