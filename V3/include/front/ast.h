/*
	* ast.h - header for ast objects
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
#include "expressions.h"

#include <stdlib.h>
#include <stdio.h>
/* --- Typedefs - Structs - Enums ---*/
typedef struct ResolvedType ResolvedType;

typedef enum {
	NODE_PROGRAM,
	NODE_VAR_DECL,
	NODE_FUNC_DECL,
	NODE_RETURN,
	NODE_EXPR,
	NODE_ENUM_DECL,
	NODE_STRUCT_DECL,
	NODE_WHILE_STMT,
	NODE_DO_WHILE_STMT,
	NODE_FOR_STMT,
	NODE_TYPE,
	NODE_IF_STMT,
	NODE_UNION_DECL,
	NODE_SWITCH_STMT,
	NODE_MISC,
	NODE_TYPEDEF,
	NODE_ARRAY,
} NodeType;

typedef struct {
	char *name;
	Expr *value;
} EnumMember;

typedef struct {
    /*
		* properties:
		* auto register const inline long restrict short signed static unsigned volatile extern
	*/
    enum { STORAGE_NONE, AUTO, REGISTER, STATIC, EXTERN } storage;
    enum { SIGN_NONE, SIGNED_T, UNSIGNED_T } sign;
    enum { LENGTH_NONE, SHORT_T, LONG_T, LONGLONG_T } length;
    int is_const;
    int is_volatile;
    int is_inline;
	int is_restrict;
	int pointer_depth;
} TypeSpec;

/*
	* Note to future self:
	* Currently, 'properties' are stored as an array of strings.
	* In a future semantic pass, this should be replaced with the above TypeSpec struct
	* that keeps track of storage, sign, qualifiers, length, and base_type properly.
	* Update: Fuck that!
	* Update: Unfuck that!
*/

typedef struct Node {
	NodeType type;
	ResolvedType *rtype;

	union {
		struct {
			struct Node **stmts;
		} program;

		struct {
			struct Node *type;
			const char *name;
			Expr *value;
		} var_decl;

		struct {
			struct Node *type;
			const char *name;
			struct Node **args;
			size_t arg_count;
			int is_prototype;
			struct Node **body;
			size_t body_count;
		} func_decl;
		
		struct {
			Expr *value;
		} return_stmt;

		struct {
			struct Expr *expr;
		} expr;

		struct {
			const char *name;
			EnumMember *members;
			size_t member_count;
		} enum_decl;

		struct {
			const char *name;
			struct Node **members;
			size_t member_count;
		} struct_decl;

		struct {
			const char *name;
			struct Node **members;
			size_t member_count;
		} union_decl;

		struct {
			struct Expr *cond;
			struct Node **body;
		} while_stmt;

		struct {
			struct Node **body;
			struct Expr *cond;
		} do_while_stmt;

		struct {
			struct Node *init;
			struct Node *cond;
			struct Expr *inc;
			struct Node **body;
		} for_stmt;

		struct {
			TypeSpec *spec;
			union {
				const char *base;
				struct Node *decl;
			};
			int is_decl;
		} type_node;

		// I am scared of if statements
		struct {
			struct Expr *if_cond;
			struct Node **if_body;
			struct Expr **elif_conds;
			struct Node ***elif_bodies; // This sure won't cause segfaults, right? Update: Fuck
			size_t elif_count;
			struct Node **else_body;
		} if_stmt;

		struct {
			struct Expr *expression;
			struct Expr **cases;        // switch (thing) {case (something))}
									    //thing and something both are expressions.
			struct Node ***case_bodies; // Oh no, not again
			size_t case_count;
			struct Node **default_body;
		} switch_stmt;

		struct {
			char *name;
		} misc;

		struct {
			char *name;
			struct Node *type;
		} typedef_node;

		struct {
			struct Node *type;     // Type node
			char *name;
			struct Expr *value;    // Expression
			size_t *dim;
			size_t dim_count;
		} array;
	};
} Node;

/* --- Globals ---*/

/* --- Prototypes ---*/
Node *create_node(NodeType type);
void free_ast(Node *node);
#endif