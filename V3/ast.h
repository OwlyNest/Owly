/*
	* ast.h - [Enter description]
	* Author:   Amity
	* Date:     Thu Oct 30 00:43:29 2025
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

/* --- Macros ---*/
#ifndef AST_H
#define AST_H
/* --- Includes ---*/
#include <stdlib.h>
/* --- Typedefs - Structs - Enums ---*/
typedef enum {
	EXPR_LITERAL,
	EXPR_IDENTIFIER,
	EXPR_BINARY_OP,
	EXPR_UNARY_OP,
} ExprKind;
typedef enum {
	NODE_VAR_DECL,
	NODE_FUNC_DECL,
	NODE_RETURN,
} NodeType;

typedef struct {
    enum { STORAGE_NONE, AUTO, REGISTER, STATIC, EXTERN } storage;
    enum { SIGN_NONE, SIGNED_T, UNSIGNED_T } sign;
    enum { LENGTH_NONE, SHORT_T, LONG_T, LONGLONG_T } length;
    int is_const;
    int is_volatile;
    const char *base_type; // "int", "char", "float", etc.
} TypeSpec;

/*
	* Note to future self:
	* Currently, 'properties' are stored as an array of strings.
	* In a future semantic pass, this should be replaced with the above TypeSpec struct
	* that keeps track of storage, sign, qualifiers, length, and base_type properly.
*/

typedef struct Node {
	NodeType type;

	union {
		struct {
			const char *properties[11]; // there are 11 properties
			const char *type;
			int is_pointer;
			const char *name;
			const char *value;
		} var_decl;

		struct {
			const char *properties[11]; // there are 11 properties
			const char *type;
			int is_pointer;
			const char *name;
			struct Node **args;
			size_t arg_count;
			int is_prototype;
			struct Node **body;
			size_t body_count;
		} func_decl;
		
		struct {
			const char *value;
		} return_stmt;

	};
} Node;

/* --- Globals ---*/

/* --- Prototypes ---*/
Node *create_node(NodeType type);
#endif