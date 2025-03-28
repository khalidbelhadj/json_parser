#include "arena.h"

#include <string.h>

static region_t *region_new(size_t capacity) {
    region_t *r =
        (region_t *)calloc(1, sizeof(region_t) + sizeof(void *) * capacity);
    *r = (region_t){
        .next = NULL,
        .capacity = capacity,
        .size = 0,
    };
    return r;
}

void *arena_alloc(Arena *a, size_t size) {
    if (a->last == NULL) {
        // No regions yet
        assert(a->first == NULL &&
               "First region is non-null when last region is null");
        a->last = region_new(size > REGION_CAPACITY ? size : REGION_CAPACITY);
        a->first = a->last;
    }

    if (a->last->capacity < a->last->size + size) {
        // Not enough space in a->end
        assert(a->last->next == NULL &&
               "Last region is non-null when adding a new region");
        a->last->next =
            region_new(size > REGION_CAPACITY ? size : REGION_CAPACITY);
        a->last = a->last->next;
    }

    void *res = &a->last->data[a->last->size];
    a->last->size += size;

    return res;
}

char *arena_alloc_str(Arena *a, const char *str) {
    if (str == NULL) {
        return NULL;
    }

    size_t len = strlen(str) + 1;  // +1 for null terminator
    char *result = (char *)arena_alloc(a, len);

    if (result != NULL) {
        memcpy(result, str, len);
    }

    return result;
}

void arena_free(Arena *a) {
    region_t *current = a->first;
    while (current != NULL) {
        region_t *tmp = current->next;
        free(current);
        current = tmp;
    }
    *a = (Arena){0};
}
