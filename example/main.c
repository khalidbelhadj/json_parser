#include "utils.h"
#include "tokeniser.h"
#include "parser.h"

JSONElement *json_object_get(JSONObject *object, char *key) {
    for (size_t i = 0; i < object->pair_count; i++) {
        if (strcmp(object->pairs[i].key, key) == 0) {
            return &object->pairs[i].value;
        }
    }
    return NULL;
}

JSONElement *json_array_get(JSONArray *array, size_t index) {
    if (index >= array->element_count) {
        return NULL;
    }
    return &array->elements[index];
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    char *file_name = argv[1];
    char *content = read_file_content(file_name);

    if (content == NULL) {
        printf("Failed to read file %s\n", file_name);
        return 1;
    }

    JSONTokeniser *tokeniser = tokenise(content);
    free(content);

    JSONParser *parser = parse(tokeniser);
    print_element(parser->root, 0);

    return 0;
}