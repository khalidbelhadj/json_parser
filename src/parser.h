#pragma once

#include "tokeniser.h"

#define LIST_OF_VALUE_TYPES(X)                                                 \
    X(JSON_VALUE_STRING)                                                       \
    X(JSON_VALUE_NUMBER_INT)                                                   \
    X(JSON_VALUE_NUMBER_FLOAT)                                                 \
    X(JSON_VALUE_BOOLEAN)                                                      \
    X(JSON_VALUE_NULL)

#define VALUE_TYPE(type) type,
typedef enum { LIST_OF_VALUE_TYPES(VALUE_TYPE) } JSONValuetype;
#undef VALUE_TYPE

#define VALUE_TYPE(type) #type,
static const char *value_type_names[] = {LIST_OF_VALUE_TYPES(VALUE_TYPE)};
#undef VALUE_TYPE

typedef struct {
    JSONValuetype type;
    union {
        char *string;
        long long number_int;
        long double number_float;
        bool boolean;
    } value;
} JSONValue;

#define LIST_OF_ELEMENT_TYPES(X)                                               \
    X(JSON_ELEMENT_OBJECT)                                                     \
    X(JSON_ELEMENT_ARRAY)                                                      \
    X(JSON_ELEMENT_VALUE)                                                      \
    X(JSON_ELEMENT_END)

#define ELEMENT_TYPE(type) type,
typedef enum { LIST_OF_ELEMENT_TYPES(ELEMENT_TYPE) } JSONElementType;
#undef ELEMENT_TYPE

#define ELEMENT_TYPE(type) #type,
static const char *element_type_names[] = {LIST_OF_ELEMENT_TYPES(ELEMENT_TYPE)};
#undef ELEMENT_TYPE

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

typedef struct JSONArray {
    JSONElement *elements;
    size_t element_count;
} JSONArray;

typedef struct {
    char *key;
    JSONElement value;
} JSONPair;

typedef struct JSONObject {
    JSONPair *pairs;
    size_t pair_count;
} JSONObject;

typedef struct {
    JSONElement root;
    JSONToken *tokens;
    size_t token_count;
    size_t current_token;
} JSONParser;

JSONElement* parse         (char *content);
char*        stringify     (JSONElement *element);
JSONParser*  new_parser    (JSONTokeniser *t);
JSONElement  parse_string  (JSONParser *p);
JSONElement  parse_true    (JSONParser *p);
JSONElement  parse_false   (JSONParser *p);
JSONElement  parse_null    (JSONParser *p);
JSONObject*  parse_object  (JSONParser *p);
JSONArray*   parse_array   (JSONParser *p);
JSONElement  parse_element (JSONParser *p);
void         print_element (JSONElement *element, int indent);
void         free_element  (JSONElement *element);