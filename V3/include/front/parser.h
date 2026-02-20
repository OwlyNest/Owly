/*
	* include/front/parser.h - parser prototypes. Owly's grammar blueprint.
	* Author:   Amity
	* Date:     Wed Feb 18 09:00:30 2026
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
#ifndef PARSER_H
#define PARSER_H
/* --- Includes ---*/
#include "lexer.h"
#include "ast.h"
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/

/* --- Prototypes ---*/
void parser_error(const char *msg, Token *t);
Token *peek(void);
Token *peek_next(void);
Token *peek_n(int n);
Token *consume(void);
int match(TokenType type, const char *lexeme);
Token *expect(TokenType expected, const char *err_msg);
void free_parser(Node *ast);
int is_at_end(void);
int is_property(TokenType type);
int is_type(TokenType type);
int is_literal(TokenType type);
int is_operator(TokenType type);
int is_unary_operator(Token *tok);
int is_binary_operator(Token *tok);
void validate_specifiers(const char **props);
TokenType type_from_string(const char *str);
Node *parser_init(TokenList *list);
Node *parse_program(void);
Node *parse_var_decl(void);
Node *parse_func_decl(void);
Node *parse_arg_decl(void);
Node *parse_ret(void);
Node **parse_block(size_t *out_count);
Node *parse_expression(void);
Node *parse_enum_decl(void);
Node *parse_struct_decl(void);
Node *parse_union_decl(void);
Node *parse_while_stmt(void);
Node *parse_do_while_stmt(void);
Node *parse_for_stmt(void);
TypeSpec *parse_type_spec(void);
Node *parse_type(void);
Node *parse_if_stmt(void);
Node *parse_switch_stmt(void);
Node *parse_misc_stmt(void);
Node *parse_typedef(void);
Node *parse_array(void);

#endif