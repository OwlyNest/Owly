/*
	* expressions.h - header file for expressions
	* Author:   Amity
	* Date:     Sat Nov  1 16:51:21 2025
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
#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H
/* --- Includes ---*/
#include "owlylexer.h"
/* --- Typedefs - Structs - Enums ---*/
// I am afraid of expressions
typedef enum {
	EXPR_LITERAL,
	EXPR_IDENTIFIER,
	EXPR_UNARY,
	EXPR_BINARY,
	EXPR_GROUPING,
	EXPR_CALL,
} ExprKind;

typedef struct Expr {
	ExprKind kind;

	union {
		// x = 5;
		char *literal;

		// x = y;
		char *identifier;

		// x++;
		struct {
			char *op;
			struct Expr *operand;
			int order; // 0 = postfix (x++), 1 = prefix (++x)
		} unary;

		// x + 1;
		struct {
			char *op;
			struct Expr *left;
			struct Expr *right;
		} binary;

		// (x + 1)
		struct {
			struct Expr *expr;
		} group;

		// add(1, 2)
		struct {
			char *func;
			struct Expr **args;
			size_t arg_count;
		} call;
	};
} Expr;

/* --- Globals ---*/

/* --- Prototypes ---*/
Expr *parse_expr(void);
Expr *parse_expression_prec(int min_prec);
Expr *parse_primary(void);
Expr *parse_unary(void);
int get_precedence(Token *tok);
void free_expr(Expr *expr);
#endif