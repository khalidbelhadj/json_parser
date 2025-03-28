/*
    Arena allocator
*/

#ifndef ARENA_H
#define ARENA_H

#include <assert.h>
#include <stdlib.h>

#define REGION_CAPACITY ((size_t)1 << 16)
#define MAX(a, b) (a > b ? a : b)

typedef struct Region {
    struct Region *next;
    size_t capacity;
    size_t size;
    void *data[];
} region_t;

typedef struct {
    region_t *first;
    region_t *last;
} Arena;

void *arena_alloc(Arena *a, size_t size);
char *arena_alloc_str(Arena *a, const char *str);
void arena_free(Arena *a);

#endif  // ARENA_H
