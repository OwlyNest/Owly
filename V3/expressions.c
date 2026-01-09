/*
	* expressions.c - [Enter description]
	* Author:   Amity
	* Date:     Sat Nov  1 16:51:23 2025
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

/* --- Includes ---*/
#include "expressions.h"
#include "ast.h"
#include "parser.h"
#include "owlylexer.h"
#include "memutils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/
/* --- Prototypes ---*/
Expr *parse_expr(void);
Expr *parse_expression_prec(int min_prec);
Expr *parse_primary(void);
Expr *parse_unary(void);
int get_precedence(Token *tok);
void free_expr(Expr *expr);

/* --- Functions ---*/
Expr *create_expr(ExprKind kind) {
	Expr *expr = xcalloc(1, sizeof(Expr));
	expr->kind = kind;
	return expr;
}

Expr *parse_expr(void) {
    return parse_expression_prec(0);
}

Expr *parse_expression_prec(int min_prec) {
    Expr *left = parse_unary();

    while (1) {
        Token *tok = peek();

        if (!tok) break;

        int prec = get_precedence(tok);
        if (prec < min_prec) break;

        if (!is_binary_operator(tok)) break;

        Token *op_tok = consume();
        Expr *right = parse_expression_prec(prec + 1);

        Expr *new_left = create_expr(EXPR_BINARY);
        new_left->binary.op = strdup(op_tok->lexeme);
        new_left->binary.left = left;
        new_left->binary.right = right;

        left = new_left;
    }
    return left;
}

Expr *parse_primary(void) {
    Token *tok = consume();
    switch (tok->type) {
        case TOKEN_LITERAL_INT:
        case TOKEN_LITERAL_CHAR:
        case TOKEN_LITERAL_STRING:
        case TOKEN_LITERAL_FLOAT: {
            Expr *expr = create_expr(EXPR_LITERAL);
            expr->literal = strdup(tok->lexeme);
            return expr;
            break;
        }
        case TOKEN_IDENTIFIER: {
            if (peek()->type == TOKEN_LPAREN) {
                Expr *expr = create_expr(EXPR_CALL);
                expr->call.func = strdup(tok->lexeme);
                consume();

                Expr **args = NULL;
                size_t arg_count = 0;

                if (peek()->type != TOKEN_RPAREN) {
                    do {
                        Expr *arg = parse_expression_prec(0);
                        if (!arg) {
                            parser_error("Expected expression in function call", peek());
                            break;
                        }
                        args = xrealloc(args, sizeof(Expr *) * (arg_count + 1));
                        args[arg_count++] = arg;
                    } while (peek()->type == TOKEN_COMMA && consume());
                }

                expect(TOKEN_RPAREN, "Expected ')' after function arguments");
                consume();
        
                expr->call.args = args;
                expr->call.arg_count = arg_count;
        
                return expr;

            } else {
                Expr *expr = create_expr(EXPR_IDENTIFIER);
                expr->identifier = strdup(tok->lexeme);
                return expr;
            }
            break;
        }

        case TOKEN_LPAREN: {
            Expr *inner = parse_expression_prec(0);

            expect(TOKEN_RPAREN, "Expected ')' after grouping");
            consume(); // consume the ')'

            Expr *group = create_expr(EXPR_GROUPING);
            group->group.expr = inner;
            return group;
            break;
        }
        default: {
            parser_error("Unexpected token in primary", tok);
        }
    }
}

Expr *parse_unary(void) {
    Token *tok = peek();
    if (!tok) return NULL;

    // Prefix unary
    if (tok->type == TOKEN_OPERATOR_INCREMENT || tok->type == TOKEN_OPERATOR_DECREMENT ||
        is_unary_operator(tok)) {
        consume();
        Expr *expr = create_expr(EXPR_UNARY);
        expr->unary.op = strdup(tok->lexeme);
        expr->unary.order = 1; // prefix
        expr->unary.operand = parse_unary(); // recursive
        return expr;
    }

    // Primary
    Expr *primary = parse_primary();
    if (!primary) return NULL;

    // Postfix unary: loop in case multiple (x++++)
    while (1) {
        tok = peek();
        if (!tok) break;

        if (tok->type == TOKEN_OPERATOR_INCREMENT || tok->type == TOKEN_OPERATOR_DECREMENT) {
            consume();
            Expr *postfix = create_expr(EXPR_UNARY);
            postfix->unary.op = strdup(tok->lexeme);
            postfix->unary.order = 0; // postfix
            postfix->unary.operand = primary;
            primary = postfix; // chain multiple postfix ops if needed
        } else {
            break;
        }
    }

    return primary;
}

int get_precedence(Token *tok) {
    switch (tok->type) {
        // 1) Assignment
        case TOKEN_OPERATOR_ASSIGN:
        case TOKEN_OPERATOR_PLUSASSIGN:
        case TOKEN_OPERATOR_MINUSASSIGN:
        case TOKEN_OPERATOR_STARASSIGN:
        case TOKEN_OPERATOR_SLASHASSIGN:
        case TOKEN_OPERATOR_PERCENTASSIGN:
        case TOKEN_OPERATOR_BITANDASSIGN:
        case TOKEN_OPERATOR_BITORASSIGN:
        case TOKEN_OPERATOR_BITXORASSIGN:
        case TOKEN_OPERATOR_BITSHLASSIGN:
        case TOKEN_OPERATOR_BITSHRASSIGN:
            return 1;

        // 2) Logical OR
        case TOKEN_OPERATOR_OR:
            return 2;

        // 3) Logical AND
        case TOKEN_OPERATOR_AND:
            return 3;

        // 4) Bitwise OR
        case TOKEN_OPERATOR_BITOR:
            return 4;

        // 5) Bitwise XOR
        case TOKEN_OPERATOR_BITXOR:
            return 5;

        // 6) Bitwise AND
        case TOKEN_OPERATOR_AMP:
            return 6;

        // 7) Equality
        case TOKEN_OPERATOR_EQUAL:
        case TOKEN_OPERATOR_NEQUAL:
            return 7;

        // 8) Relational
        case TOKEN_OPERATOR_LOWER:
        case TOKEN_OPERATOR_GREATER:
        case TOKEN_OPERATOR_LEQ:
        case TOKEN_OPERATOR_GEQ:
            return 8;

        // 9) Bit shifts
        case TOKEN_OPERATOR_BITSHL:
        case TOKEN_OPERATOR_BITSHR:
            return 9;

        // 10) Additive
        case TOKEN_OPERATOR_PLUS:
        case TOKEN_OPERATOR_MINUS:
            return 10;

        // 11) Multiplicative
        case TOKEN_OPERATOR_STAR:
        case TOKEN_OPERATOR_SLASH:
        case TOKEN_OPERATOR_PERCENT:
            return 11;

        // 12) Unary
        case TOKEN_OPERATOR_NOT:
        case TOKEN_OPERATOR_BITNOT:
        case TOKEN_OPERATOR_INCREMENT:
        case TOKEN_OPERATOR_DECREMENT:
            return 12;

        // 13) Member access / pointer access
        case TOKEN_OPERATOR_POINT:
        case TOKEN_OPERATOR_ARROW:
            return 13;
			
        default:
            return 0;
    }
}

void free_expr(Expr *expr) {
    if (!expr) return;

    switch (expr->kind) {
        case EXPR_BINARY:
            free_expr(expr->binary.left);
            free_expr(expr->binary.right);
            xfree(expr->binary.op);
            break;
        case EXPR_UNARY:
            free_expr(expr->unary.operand);
            xfree(expr->unary.op);
            break;
        case EXPR_LITERAL:
            xfree((void *)expr->literal);
            break;
        case EXPR_IDENTIFIER:
            xfree(expr->identifier);
            break;
        case EXPR_GROUPING:
            free_expr(expr->group.expr);
            break;
        case EXPR_CALL:
            xfree(expr->call.func);
            for (size_t i = 0; i < expr->call.arg_count; i++) {
                free_expr(expr->call.args[i]);
            }
            xfree(expr->call.args);
            break;
    }
    xfree(expr);
}