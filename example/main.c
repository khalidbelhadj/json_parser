#include <time.h>

#include "../include/parser.h"
#include "../include/tokenizer.h"

// JSONElement *json_object_get(JSONObject *object, char *key) {
//     for (size_t i = 0; i < object->pair_count; i++) {
//         if (strcmp(object->first[i].key, key) == 0) {
//             return &object->first[i].value;
//         }
//     }
//     return NULL;
// }

// JSONElement *json_array_get(JSONArray *array, size_t index) {
//     if (index >= array->element_count) {
//         return NULL;
//     }
//     return &array->first[index];
// }

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    char *file_name = argv[1];

    Arena a = {0};
    int error = 0;

    clock_t t;
    t = clock();

    JSONElement json = json_parse_file(&a, file_name, &error);
    (void)json;
    if (error != 0) {
        return 1;
    }

    t = clock() - t;
    printf("done %f\n", (double)t / CLOCKS_PER_SEC);
    return 0;
}