#include <stdio.h>
#include <stdlib.h>
#include "memutils.h"

void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        perror("malloc");
        exit(1);
    }
    return ptr;
}

void *xrealloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        perror("realloc");
        exit(1);
    }
    return new_ptr;
}