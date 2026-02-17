/*
	* src/memutils.c - Owly's dramatic but reliable memory butler
	* Author:   Amity
	* Date:     Tue Feb 17 08:54:33 2026
	* Copyright © 2026 OwlyNest
*/

/* --- Styling Instructions ---
	* Encoding:                      UTF-8, Unix line endings
	* Text font:                     Monospace
	* Line width:                    Max 80 characters
	* Indentation:                   Use 4 spaces
	* Brace style:                   Same line as control statement
	* Inline comments:               Column 40, wherever possible, else, whole multiple of 20
	* Section headers:               Use 3 '-' characters before and after
	* Pointer notation:              Next to variable name, not type
	* Binary operations:             Space around operator
	* Empty parameter list:          Use (void) instead of ()
	* Statements and declarations:   Max one per line
*/

/* --- Macros ---*/

/* --- Includes ---*/
#include <stdio.h>
#include <stdlib.h>

#include "memutils.h"

/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/
size_t total_allocated = 0;
extern int debug;
/* --- Prototypes ---*/
void *xmalloc(size_t size);
void *xcalloc(size_t n, size_t size);
void *xrealloc(void *ptr, size_t size);
void xfree(void *ptr);
/* --- Functions ---*/

/*
	xmalloc - Allocates memory or throws a tantrum and exits.
	No nullptr returns here,  if we can't get memory, we die dramatically.
*/
void *xmalloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        perror("malloc: Out of memory. Owly is very disappointed.");
        exit(1);
    }
    return ptr;
}


/*
	xcalloc - Zero-initialized allocation. Same drama as xmalloc.
*/
void *xcalloc(size_t n, size_t size) {
    void *ptr = calloc(n, size);
    if (!ptr) {
        perror("calloc: Zeroed memory allocation failed. Hoot hoot, panic.");
        exit(1);
    }
    return ptr;
}

/*
	xrealloc - Resizes memory or gives up like a diva.
*/
void *xrealloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        perror("xrealloc: Memory resize failed. Owly refuses to continue.");
        exit(1);
    }
    return new_ptr;
}


/*
	xfree - Frees memory safely. Does NOT set pointer to NULL (that's caller's job).
	We trust you not to double-free. If you do... well, that's on you, darling.
*/
void xfree(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}