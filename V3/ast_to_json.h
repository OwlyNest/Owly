/*
	* ast_to_json.h - [Enter description]
	* Author:   Amity
	* Date:     Thu Oct 30 14:59:18 2025
	* Copyright © 2025 OwlyNest
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
#ifndef ASTTOJSON_H
#define ASTTOJSON_H
/* --- Macros ---*/

/* --- Includes ---*/
#include "ast.h"
#include "parser.h"

#include <cjson/cJSON.h>
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/

/* --- Prototypes ---*/
cJSON *var_decl_to_json(Node *decl);
cJSON *func_decl_to_json(Node *node);
#endif