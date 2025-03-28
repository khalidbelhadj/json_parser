#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

char *read_file_content(const char *file_name);
bool is_whitespace(char c);
bool is_digit(char c);
