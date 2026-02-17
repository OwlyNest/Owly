
#ifndef MEMUTILS_H
#define MEMUTILS_H

#include <stddef.h>

void *xmalloc(size_t size);
void *xcalloc(size_t n, size_t size);
void *xrealloc(void *prt, size_t size);
void xfree(void *ptr);

#endif
