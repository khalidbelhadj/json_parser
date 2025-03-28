#include "../include/utils.h"

char *read_file_content(const char *file_name) {
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = calloc(1, file_size + 1);
    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0';

    fclose(file);
    return file_content;
}

bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool is_digit(char c) { return c >= '0' && c <= '9'; }