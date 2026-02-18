/*
	* include/memutils.h - Memory helpers. Owly promises not to leak... much.
	* Author:   Amity
	* Date:     Wed Feb 18 08:58:33 2026
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
#ifndef MEMUTILS_H
#define MEMUTILS_H
/* --- Includes ---*/
#include <stddef.h>
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/

/* --- Prototypes ---*/
void *xmalloc(size_t size);
void *xcalloc(size_t n, size_t size);
void *xrealloc(void *prt, size_t size);
void xfree(void *ptr);

#endif
