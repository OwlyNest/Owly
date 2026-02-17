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
#include "lexer.h"
/* --- Typedefs - Structs - Enums ---*/
typedef struct ResolvedType ResolvedType;
// I am afraid of expressions
typedef enum {
	LIT_INT,
	LIT_FLOAT,
	LIT_CHAR,
	LIT_STRING,
	LIT_BOOL,
} LiteralKind;

typedef struct {
	LiteralKind *kind;
	char *raw;

	union {
		long long int_val;
		double float_val;
		char char_val;
		char *string_val;
		int bool_val;
	};
} Literal;

typedef enum {
	EXPR_LITERAL,
	EXPR_IDENTIFIER,
	EXPR_UNARY,
	EXPR_BINARY,
	EXPR_GROUPING,
	EXPR_CALL,
	EXPR_TERNARY,
	EXPR_MEMBER,
	EXPR_SIZEOF,
	EXPR_CAST,
	EXPR_SET,
	EXPR_INDEX,
} ExprKind;

typedef struct Expr {
	ExprKind kind;
	ResolvedType *inferred_type;

	union {
		// x = 5;
		Literal *literal;

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

		struct {
			struct Expr *cond;
			struct Expr *true_expr;
			struct Expr *false_expr;
		} ternary;
		
		struct {
			struct Expr *object;
			char *member;
			int is_arrow;
			size_t offset;
		} member;

		struct {
			struct Expr *expr;
			struct Node *type;
			int is_type;
			size_t computed_size;
		} sizeof_expr;

		struct {
			struct Node *target_type;
			struct Expr *expr;
		} cast;

		struct {
			struct Expr **elements;
			size_t element_count;
		} set;

		struct {
			struct Expr *array;
			struct Expr *index;
		} index;
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