/*
	* parser.c - Parser: builds the AST. Owly's grammar police — strict but fair.
	* Author:   Amity
	* Date:     Wed Oct 29 14:59:59 2025
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
#define LINEMAX 1024
#define MAX_TOKENS 1024
/* --- Includes ---*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

#include "front/parser.h"
#include "front/lexer.h"
#include "front/ast.h"
#include "front/ast_to_json.h"
#include "front/expressions.h"
#include "memutils.h"
/* --- Globals ---*/
Token token_list[MAX_TOKENS];
size_t token_count = 0;
size_t pos = 0;

extern int debug;
/* --- Protypes --- */
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
Node *parser_init(void);
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

/* --- Functions --- */
void parser_error(const char *msg, Token *t) {
    if (t) {
        fprintf(stderr, "[!] Parser error: %s at token '%.*s', position %d\n", msg, (int)t->length, t->lexeme, (int)pos);
        exit(1);
    } else {
        fprintf(stderr, "[!] Parser error: %s\n", msg);
        exit(1);
    }
}

Token *peek(void) {
    if (pos < token_count) return &token_list[pos];
    return NULL;
}

Token *peek_next(void) {
    if (pos + 1 < token_count) return &token_list[pos + 1];
    return NULL;
}

Token *peek_n(int n) {
    if (pos + n < token_count) return &token_list[pos + n];
    return NULL;
}

Token *consume(void) {
    if (pos < token_count) return &token_list[pos++];
    return NULL;
}

int match(TokenType type, const char *lexeme) {
    Token *t = peek();
    if (t && t->type == type && (!lexeme || 
        (strlen(lexeme) == t->length && strncmp(t->lexeme, lexeme, t->length) == 0))) {
        consume();
        return 1;
    }
    return 0;
}

Token *expect(TokenType expected, const char *err_msg) {
    Token *tok = peek();
    if (!tok || tok->type != expected) {
        parser_error(err_msg, tok);
    }
    return tok;
}

void free_parser(Node *ast) {
    free_ast(ast);
    for (size_t i = 0; i < token_count; i++) {
        xfree((void *)token_list[i].lexeme);
    }
    printf("[X] Freed AST and token list successfully\n");
}


int is_at_end(void) {
    return pos >= token_count || token_list[pos].type == TOKEN_EOF;
}

int is_property(TokenType type) {
    switch (type) {
        case TOKEN_KEYWORD_AUTO:
        case TOKEN_KEYWORD_REGISTER:
        case TOKEN_KEYWORD_CONST:
        case TOKEN_KEYWORD_ENUM:
        case TOKEN_KEYWORD_EXTERN:
        case TOKEN_KEYWORD_INLINE:
        case TOKEN_KEYWORD_LONG:
        case TOKEN_KEYWORD_RESTRICT:
        case TOKEN_KEYWORD_SHORT:
        case TOKEN_KEYWORD_SIGNED:
        case TOKEN_KEYWORD_STATIC:
        case TOKEN_KEYWORD_STRUCT:
        case TOKEN_KEYWORD_UNSIGNED:
        case TOKEN_KEYWORD_VOLATILE:
            return 1;
        default:
            return 0;
    }
}

int is_type(TokenType type) {
    switch (type) {
        case TOKEN_KEYWORD_CHAR:
        case TOKEN_KEYWORD_DOUBLE:
        case TOKEN_KEYWORD_FLOAT:
        case TOKEN_KEYWORD_INT:
        case TOKEN_KEYWORD_VOID:
        case TOKEN_KEYWORD_BOOL:
        case TOKEN_KEYWORD_COMPLEX:
        case TOKEN_KEYWORD_IMAGINARY:
            return 1;
        default:
            return 0;
    }
}

int is_literal(TokenType type) {
    switch (type) {
        case TOKEN_LITERAL_CHAR:
        case TOKEN_LITERAL_FLOAT:
        case TOKEN_LITERAL_INT:
        case TOKEN_LITERAL_STRING:
            return 1;
        default:
            return 0;
    }
}

int is_operator(TokenType type) {
    switch (type) {
        case TOKEN_OPERATOR_PLUS:               // +
        case TOKEN_OPERATOR_MINUS:              // -
        case TOKEN_OPERATOR_STAR:               // *
        case TOKEN_OPERATOR_SLASH:              // /
        case TOKEN_OPERATOR_PERCENT:            // %
        case TOKEN_OPERATOR_INCREMENT:          // ++
        case TOKEN_OPERATOR_DECREMENT:          // --
        case TOKEN_OPERATOR_ASSIGN:             // =
        case TOKEN_OPERATOR_PLUSASSIGN:         // +=
        case TOKEN_OPERATOR_MINUSASSIGN:        // -=
        case TOKEN_OPERATOR_STARASSIGN:         // *=
        case TOKEN_OPERATOR_SLASHASSIGN:        // /=
        case TOKEN_OPERATOR_PERCENTASSIGN:      // %=
        case TOKEN_OPERATOR_EQUAL:              // ==
        case TOKEN_OPERATOR_NEQUAL:             // !=
        case TOKEN_OPERATOR_GREATER:            // >
        case TOKEN_OPERATOR_LOWER:              // <
        case TOKEN_OPERATOR_GEQ:                // >=
        case TOKEN_OPERATOR_LEQ:                // <=
        case TOKEN_OPERATOR_NOT:                // !
        case TOKEN_OPERATOR_AND:                // &&
        case TOKEN_OPERATOR_OR:                 // ||
        case TOKEN_OPERATOR_AMP:                // &
        case TOKEN_OPERATOR_BITOR:              // |
        case TOKEN_OPERATOR_BITXOR:             // ^
        case TOKEN_OPERATOR_BITNOT:             // ~
        case TOKEN_OPERATOR_BITSHL:             // <<
        case TOKEN_OPERATOR_BITSHR:             // >>
        case TOKEN_OPERATOR_BITANDASSIGN:       // &=
        case TOKEN_OPERATOR_BITORASSIGN:        // |=
        case TOKEN_OPERATOR_BITXORASSIGN:       // ^=
        case TOKEN_OPERATOR_BITSHLASSIGN:       // <<=
        case TOKEN_OPERATOR_BITSHRASSIGN:       // >>=
        case TOKEN_OPERATOR_POINT:              // .
        case TOKEN_OPERATOR_ARROW:              // ->
        case TOKEN_OPERATOR_ELLIPSIS:           // ...
            return 1;
        default:
            return 0;
    }
}

int is_unary_operator(Token *tok) {
    if (!tok) return 0;
    switch (tok->type) {
        case TOKEN_OPERATOR_INCREMENT:
        case TOKEN_OPERATOR_DECREMENT:
        case TOKEN_OPERATOR_MINUS:
        case TOKEN_OPERATOR_PLUS:
        case TOKEN_OPERATOR_NOT:
        case TOKEN_OPERATOR_BITNOT:
        case TOKEN_OPERATOR_STAR:
        case TOKEN_OPERATOR_AMP:
            return 1;
        default:
            return 0;
    }
}

int is_binary_operator(Token *tok) {
    switch (tok->type) {
        case TOKEN_OPERATOR_PLUS:
        case TOKEN_OPERATOR_MINUS:
        case TOKEN_OPERATOR_STAR:
        case TOKEN_OPERATOR_SLASH:
        case TOKEN_OPERATOR_PERCENT:
        case TOKEN_OPERATOR_AND:
        case TOKEN_OPERATOR_OR:
        case TOKEN_OPERATOR_AMP:
        case TOKEN_OPERATOR_BITOR:
        case TOKEN_OPERATOR_BITXOR:
        case TOKEN_OPERATOR_BITSHL:
        case TOKEN_OPERATOR_BITSHR:
        case TOKEN_OPERATOR_EQUAL:
        case TOKEN_OPERATOR_NEQUAL:
        case TOKEN_OPERATOR_LEQ:
        case TOKEN_OPERATOR_GEQ:
        case TOKEN_OPERATOR_LOWER:
        case TOKEN_OPERATOR_GREATER:
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
        case TOKEN_OPERATOR_ARROW:
        case TOKEN_OPERATOR_POINT:
            return 1;
        default:
            return 0;
    }
}

void validate_specifiers(const char **props) {
    int saw_signed = 0;
    int saw_unsigned = 0;
    int saw_short = 0;
    int saw_long = 0;
    int saw_storage = 0;

    for (int i = 0; props[i]; i++) {
        const char *p = props[i];
        if (!strncmp(p, "SIGNED", sizeof("SIGNED"))) saw_signed++;
        else if (!strncmp(p, "UNSIGNED", sizeof("UNSIGNED"))) saw_unsigned++;
        else if (!strncmp(p, "SHORT", sizeof("SHORT"))) saw_short++;
        else if (!strncmp(p, "LONG", sizeof("LONG"))) saw_long++;
        if (!strncmp(p, "AUTO", sizeof("AUTO")) || !strncmp(p, "REGISTER", sizeof("REGISTER")) || !strncmp(p, "STATIC", sizeof("STATIC")) || !strncmp(p, "EXTERN", sizeof("EXTERN"))) {
            saw_storage++;
        }

        if (saw_signed && saw_unsigned) {
            parser_error("cannot combine 'signed' and 'unsigned' in the same declaration", peek());
        }
        if (saw_long && saw_short) {
            parser_error("cannot combine 'long' and 'short' in the same declaration", peek());
        }
        if (saw_long > 2) {
            parser_error("Too many 'long' specifiers", peek());
        }
        if (saw_long == 2) {
            printf("[INFO]: 'long long' type detected\n");
        }

        if (saw_storage > 1) {
            parser_error("multiple storage specifiers in one declaration", peek());
        }
    }
}

TokenType type_from_string(const char *str) {
    if (strncmp(str, "EOF", sizeof("EOF")) == 0) return TOKEN_EOF;
    else if (strncmp(str, "UNKNOWN", sizeof("UNKNOWN")) == 0) return TOKEN_UNKNOWN;

    else if (strncmp(str, "AUTO", sizeof("AUTO")) == 0) return TOKEN_KEYWORD_AUTO;
    else if (strncmp(str, "ARR", sizeof("ARR")) == 0) return TOKEN_KEYWORD_ARR;
    else if (strncmp(str, "BREAK", sizeof("BREAK")) == 0) return TOKEN_KEYWORD_BREAK;
    else if (strncmp(str, "CASE", sizeof("CASE")) == 0)  return TOKEN_KEYWORD_CASE;
    else if (strncmp(str, "CHAR", sizeof("CHAR")) == 0) return TOKEN_KEYWORD_CHAR;
    else if (strncmp(str, "CONST", sizeof("CONST")) == 0) return TOKEN_KEYWORD_CONST;
    else if (strncmp(str, "CONTINUE", sizeof("CONTINUE")) == 0) return TOKEN_KEYWORD_CONTINUE;
    else if (strncmp(str, "DEFAULT", sizeof("DEFAULT")) == 0) return TOKEN_KEYWORD_DEFAULT;
    else if (strncmp(str, "DO", sizeof("DO")) == 0) return TOKEN_KEYWORD_DO;
    else if (strncmp(str, "DOUBLE", sizeof("DOUBLE")) == 0) return TOKEN_KEYWORD_DOUBLE;
    else if (strncmp(str, "ELSE", sizeof("ELSE")) == 0) return TOKEN_KEYWORD_ELSE;
    else if (strncmp(str, "ENUM", sizeof("ENUM")) == 0) return TOKEN_KEYWORD_ENUM;
    else if (strncmp(str, "EXTERN", sizeof("EXTERN")) == 0) return TOKEN_KEYWORD_EXTERN;
    else if (strncmp(str, "FLOAT", sizeof("FLOAT")) == 0) return TOKEN_KEYWORD_FLOAT;
    else if (strncmp(str, "FOR", sizeof("FOR")) == 0) return TOKEN_KEYWORD_FOR;
    else if (strncmp(str, "FUNC", sizeof("FUNC")) == 0) return TOKEN_KEYWORD_FUNC;
    else if (strncmp(str, "IF", sizeof("IF")) == 0) return TOKEN_KEYWORD_IF;
    else if (strncmp(str, "INLINE", sizeof("INLINE")) == 0) return TOKEN_KEYWORD_INLINE;
    else if (strncmp(str, "INT", sizeof("INT")) == 0) return TOKEN_KEYWORD_INT;
    else if (strncmp(str, "LONG", sizeof("LONG")) == 0) return TOKEN_KEYWORD_LONG;
    else if (strncmp(str, "REGISTER", sizeof("REGISTER")) == 0) return TOKEN_KEYWORD_REGISTER;
    else if (strncmp(str, "RESTRICT", sizeof("RESTRICT")) == 0) return TOKEN_KEYWORD_RESTRICT;
    else if (strncmp(str, "RETURN", sizeof("RETURN")) == 0) return TOKEN_KEYWORD_RETURN;
    else if (strncmp(str, "SHORT", sizeof("SHORT")) == 0) return TOKEN_KEYWORD_SHORT;
    else if (strncmp(str, "SIGNED", sizeof("SIGNED")) == 0) return TOKEN_KEYWORD_SIGNED;
    else if (strncmp(str, "SIZEOF", sizeof("SIZEOF")) == 0) return TOKEN_KEYWORD_SIZEOF;
    else if (strncmp(str, "STATIC", sizeof("STATIC")) == 0) return TOKEN_KEYWORD_STATIC;
    else if (strncmp(str, "STRUCT", sizeof("STRUCT")) == 0) return TOKEN_KEYWORD_STRUCT;
    else if (strncmp(str, "SWITCH", sizeof("SWITCH")) == 0) return TOKEN_KEYWORD_SWITCH;
    else if (strncmp(str, "TYPEDEF", sizeof("TYPEDEF")) == 0) return TOKEN_KEYWORD_TYPEDEF;
    else if (strncmp(str, "UNION", sizeof("UNION")) == 0) return TOKEN_KEYWORD_UNION;
    else if (strncmp(str, "UNSIGNED", sizeof("UNSIGNED")) == 0) return TOKEN_KEYWORD_UNSIGNED;
    else if (strncmp(str, "VAR", sizeof("VAR")) == 0) return TOKEN_KEYWORD_VAR;
    else if (strncmp(str, "VOID", sizeof("VOID")) == 0) return TOKEN_KEYWORD_VOID;
    else if (strncmp(str, "VOLATILE", sizeof("VOLATILE")) == 0) return TOKEN_KEYWORD_VOLATILE;
    else if (strncmp(str, "WHILE", sizeof("WHILE")) == 0) return TOKEN_KEYWORD_WHILE;
    else if (strncmp(str, "_BOOL", sizeof("_BOOL")) == 0) return TOKEN_KEYWORD_BOOL;
    else if (strncmp(str, "_COMPLEX", sizeof("_COMPLEX")) == 0) return TOKEN_KEYWORD_COMPLEX;
    else if (strncmp(str, "_IMAGINARY", sizeof("_IMAGINARY")) == 0) return TOKEN_KEYWORD_IMAGINARY;

    else if (strncmp(str, "PLUS", sizeof("PLUS")) == 0) return TOKEN_OPERATOR_PLUS;
    else if (strncmp(str, "MINUS", sizeof("MINUS")) == 0) return TOKEN_OPERATOR_MINUS;
    else if (strncmp(str, "STAR", sizeof("STAR")) == 0) return TOKEN_OPERATOR_STAR;
    else if (strncmp(str, "SLASH", sizeof("SLASH")) == 0) return TOKEN_OPERATOR_SLASH;
    else if (strncmp(str, "PERCENT", sizeof("PERCENT")) == 0) return TOKEN_OPERATOR_PERCENT;
    else if (strncmp(str, "INCREMENT", sizeof("INCREMENT")) == 0) return TOKEN_OPERATOR_INCREMENT;
    else if (strncmp(str, "DECREMENT", sizeof("DECREMENT")) == 0) return TOKEN_OPERATOR_DECREMENT;
    else if (strncmp(str, "ASSIGN", sizeof("ASSIGN")) == 0) return TOKEN_OPERATOR_ASSIGN;
    else if (strncmp(str, "PLUS_ASSIGN", sizeof("PLUS_ASSIGN")) == 0) return TOKEN_OPERATOR_PLUSASSIGN;
    else if (strncmp(str, "MINUS_ASSIGN", sizeof("MINUS_ASSIGN")) == 0) return TOKEN_OPERATOR_MINUSASSIGN;
    else if (strncmp(str, "STAR_ASSIGN", sizeof("STAR_ASSIGN")) == 0) return TOKEN_OPERATOR_STARASSIGN;
    else if (strncmp(str, "SLASH_ASSIGN", sizeof("SLASH_ASSIGN")) == 0) return TOKEN_OPERATOR_SLASHASSIGN;
    else if (strncmp(str, "PERCENT_ASSIGN", sizeof("PERCENT_ASSIGN")) == 0) return TOKEN_OPERATOR_PERCENTASSIGN;
    else if (strncmp(str, "EQUAL", sizeof("EQUAL")) == 0) return TOKEN_OPERATOR_EQUAL;
    else if (strncmp(str, "NOT_EQUAL", sizeof("NOT_EQUAL")) == 0) return TOKEN_OPERATOR_NEQUAL;
    else if (strncmp(str, "GREATER", sizeof("GREATER")) == 0) return TOKEN_OPERATOR_GREATER;
    else if (strncmp(str, "LOWER", sizeof("LOWER")) == 0) return TOKEN_OPERATOR_LOWER;
    else if (strncmp(str, "GREATER_EQUAL", sizeof("GREATER_EQUAL")) == 0) return TOKEN_OPERATOR_GEQ;
    else if (strncmp(str, "LOWER_EQUAL", sizeof("LOWER_EQUAL")) == 0) return TOKEN_OPERATOR_LEQ;
    else if (strncmp(str, "NOT", sizeof("NOT")) == 0) return TOKEN_OPERATOR_NOT;
    else if (strncmp(str, "AND", sizeof("AND")) == 0) return TOKEN_OPERATOR_AND;
    else if (strncmp(str, "OR", sizeof("OR")) == 0) return TOKEN_OPERATOR_OR;
    else if (strncmp(str, "AMPERSAND", sizeof("AMPERSAND")) == 0) return TOKEN_OPERATOR_AMP;
    else if (strncmp(str, "BIT_OR", sizeof("BIT_OR")) == 0) return TOKEN_OPERATOR_BITOR;
    else if (strncmp(str, "BIT_XOR", sizeof("BIT_XOR")) == 0) return TOKEN_OPERATOR_BITXOR;
    else if (strncmp(str, "BIT_NOT", sizeof("BIT_NOT")) == 0) return TOKEN_OPERATOR_BITNOT;
    else if (strncmp(str, "SHL", sizeof("SHL")) == 0) return TOKEN_OPERATOR_BITSHL;
    else if (strncmp(str, "SHR", sizeof("SHR")) == 0) return TOKEN_OPERATOR_BITSHR;
    else if (strncmp(str, "AND_ASSIGN", sizeof("AND_ASSIGN")) == 0) return TOKEN_OPERATOR_BITANDASSIGN;
    else if (strncmp(str, "OR_ASSIGN", sizeof("OR_ASSIGN")) == 0) return TOKEN_OPERATOR_BITORASSIGN;
    else if (strncmp(str, "BITXOR_ASSIGN", sizeof("BITXOR_ASSIGN")) == 0) return TOKEN_OPERATOR_BITXORASSIGN;
    else if (strncmp(str, "SHL_ASSIGN", sizeof("SHL_ASSIGN")) == 0) return TOKEN_OPERATOR_BITSHLASSIGN;
    else if (strncmp(str, "SHR_ASSIGN", sizeof("SHR_ASSIGN")) == 0) return TOKEN_OPERATOR_BITSHRASSIGN;
    else if (strncmp(str, "POINT", sizeof("POINT")) == 0) return TOKEN_OPERATOR_POINT;
    else if (strncmp(str, "ARROW", sizeof("ARROW")) == 0) return TOKEN_OPERATOR_ARROW;
    else if (strncmp(str, "ELLIPSIS", sizeof("ELLIPSIS")) == 0) return TOKEN_OPERATOR_ELLIPSIS;

    else if (strncmp(str, "LEFT_PARENTHESIS", sizeof("LEFT_PARENTHESIS")) == 0) return TOKEN_LPAREN;
    else if (strncmp(str, "RIGHT_PARENTHESIS", sizeof("RIGHT_PARENTHESIS")) == 0) return TOKEN_RPAREN;
    else if (strncmp(str, "LEFT_BRACKET", sizeof("LEFT_BRACKET")) == 0) return TOKEN_LBRACKET;
    else if (strncmp(str, "RIGHT_BRACKET", sizeof("RIGHT_BRACKET")) == 0) return TOKEN_RBRACKET;
    else if (strncmp(str, "LEFT_BRACE", sizeof("LEFT_BRACE")) == 0) return TOKEN_LBRACE;
    else if (strncmp(str, "RIGHT_BRACE", sizeof("RIGHT_BRACE")) == 0) return TOKEN_RBRACE;

    else if (strncmp(str, "COMMA", sizeof("COMMA")) == 0) return TOKEN_COMMA;
    else if (strncmp(str, "COLON", sizeof("COLON")) == 0) return TOKEN_COLON;
    else if (strncmp(str, "SEMICOLON", sizeof("SEMICOLON")) == 0) return TOKEN_SEMICOLON;
    else if (strncmp(str, "QMARK", sizeof("QMARK")) == 0) return TOKEN_QUESTION;
    else if (strncmp(str, "HASH", sizeof("HASH")) == 0) return TOKEN_HASH;
    
    else if (strncmp(str, "IDENTIFIER", sizeof("IDENTIFIER")) == 0) return TOKEN_IDENTIFIER;

    else if (strncmp(str, "STRING_LITERAL", sizeof("STRING_LITERAL")) == 0) return TOKEN_LITERAL_STRING;
    else if (strncmp(str, "CHAR_LITERAL", sizeof("CHAR_LITERAL")) == 0) return TOKEN_LITERAL_CHAR;
    else if (strncmp(str, "INT_LITERAL", sizeof("INT_LITERAL")) == 0) return TOKEN_LITERAL_INT;
    else if (strncmp(str, "FLOAT_LITERAL", sizeof("FLOAT_LITERAL")) == 0) return TOKEN_LITERAL_FLOAT;
    else return TOKEN_UNKNOWN;
}

Node *parser_init(void) {
    char type_str[64];
    char buf[LINEMAX];
    FILE *tokens = fopen("out/list.tok", "r");
    while (fscanf(tokens, "%63[^,], \"%[^\"]\";\n", type_str, buf) == 2) {
        Token tok;
        tok.type = type_from_string(type_str);
        tok.lexeme = strdup(buf);
        tok.length = (size_t)strlen(buf);
        token_list[token_count++] = tok;
    }
    fclose(tokens);

    Node *ast = parse_program();
    create_json(ast);
    return ast;
}

Node *parse_program(void) {
    Node *program = create_node(NODE_PROGRAM);
    size_t stmt_count = 0;
    while (peek() && (peek()->type != TOKEN_EOF)) {
        program->program.stmts = parse_block(&stmt_count);
    }
    return program;
}

Node *parse_var_decl(void) {
    /*
        * Pattern:
        * var [properties] type [*]ident [= literal|identifier|&identifier]
    */
    Node *node = create_node(NODE_VAR_DECL);
    node->var_decl.type = parse_type();
    node->var_decl.value = NULL;

    // Name
    expect(TOKEN_IDENTIFIER, "Expected variable name after type");
    Token *name = consume();
    node->var_decl.name = strdup(name->lexeme);

    if (peek()->type != TOKEN_SEMICOLON) {
        if (peek() && (peek()->type == TOKEN_OPERATOR_ASSIGN)) {
            consume();
            node->var_decl.value = parse_expr();   
        }
    }

    expect(TOKEN_SEMICOLON, "Expected ';' after variable declaration");
    consume();
    return node;
}

Node *parse_func_decl(void) {
    /*
        * Pattern:
        * func [properties] type [*]ident([void| typeargs,]) {[body]}
    */

    Node *node = create_node(NODE_FUNC_DECL);

    node->func_decl.type = parse_type();

    // Name
    expect(TOKEN_IDENTIFIER, "Expected function name after type");
    Token *name = consume();
    node->func_decl.name = strdup(name->lexeme);

    expect(TOKEN_LPAREN, "Expected opening parenthesis after function name");
    consume();

    node->func_decl.args = NULL;
    node->func_decl.arg_count = 0;

    while (peek()->type != TOKEN_RPAREN && peek()->type != TOKEN_EOF) {
        // special case `void` except void*
        if (peek()->type == TOKEN_KEYWORD_VOID && peek_next()->type != TOKEN_OPERATOR_STAR) {
            consume();
            if (peek()->type != TOKEN_RPAREN) {
                parser_error("Expected closing parenthesis after `void`", peek());
            }
            break;
        }
        Node *arg = parse_arg_decl();

        node->func_decl.args = xrealloc(
            node->func_decl.args,
            sizeof(Node *) * (node->func_decl.arg_count + 1)
        );
        node->func_decl.args[node->func_decl.arg_count++] = arg;

        if (peek()->type == TOKEN_COMMA) {
            consume(); // skip comma
        } else if (peek()->type != TOKEN_RPAREN) {
            parser_error("Expected ',' or ')' in function argument list", peek());
        }
    }

    expect(TOKEN_RPAREN, "Expected closing parenthesis");
    consume();

    if (peek()->type == TOKEN_SEMICOLON) {
        // Prototype
        node->func_decl.is_prototype = 1;
        node->func_decl.body = NULL;
        node->func_decl.body_count = 0;
        consume();        
        return node;
    }

    expect(TOKEN_LBRACE, "Expected '{' before function body");
    consume();

    node->func_decl.body = parse_block(&node->func_decl.body_count);

    expect(TOKEN_RBRACE, "Expected '}' after function body");
    consume();
    return node;
}

Node *parse_arg_decl(void) {
    /*
        * Pattern:
        * [properties] type [*]ident
    */
    Node *node = create_node(NODE_VAR_DECL);

    node->var_decl.type = parse_type();
    // Name
    expect(TOKEN_IDENTIFIER, "Expected variable name after type");
    
    Token *name = consume();
    node->var_decl.name = strdup(name->lexeme);

    if (peek()->type != TOKEN_SEMICOLON) {
        if (peek() && (peek()->type == TOKEN_OPERATOR_ASSIGN)) {
            consume();
            node->var_decl.value = parse_expr();   
        }
    }
    return node;
}

Node *parse_ret(void) {
    Node *node = create_node(NODE_RETURN);

    if (peek()->type != TOKEN_SEMICOLON) {
        node->return_stmt.value = parse_expr();
    } else {
        node->return_stmt.value = NULL;
    }
    
    expect(TOKEN_SEMICOLON, "Expected ';' after return statement");
    consume();
    return node;
}

Node **parse_block(size_t *out_count) {
    /*
        * Pattern:
        * unnecessary
    */
    Node **body = NULL;
    *out_count = 0;

    while (1) {
        Token *tok = peek();
        if (!tok || tok->type == TOKEN_EOF || tok->type == TOKEN_RBRACE) {
            break;
        }
        Node *stmt = NULL;
        if (tok->type == TOKEN_KEYWORD_VAR) {
            consume();
            stmt = parse_var_decl();
        } else if (tok->type == TOKEN_KEYWORD_FUNC) {
            consume();
            stmt = parse_func_decl();
        } else if (tok->type == TOKEN_KEYWORD_RETURN) {
            consume();
            stmt = parse_ret();
        } else if (tok->type == TOKEN_IDENTIFIER || is_operator(tok->type) || is_literal(tok->type) || tok->type == TOKEN_LPAREN) {
            stmt = parse_expression();
        } else if (tok->type == TOKEN_KEYWORD_ENUM){
            consume();
            stmt = parse_enum_decl();
        } else if (tok->type == TOKEN_KEYWORD_STRUCT) {
            consume();
            stmt = parse_struct_decl();
        } else if (tok->type == TOKEN_KEYWORD_WHILE) {
            consume();
            stmt = parse_while_stmt();
        } else if (tok->type == TOKEN_KEYWORD_DO) {
            consume();
            stmt = parse_do_while_stmt();
        } else if (tok->type == TOKEN_KEYWORD_FOR) {
            consume();
            stmt = parse_for_stmt();
        } else if (tok->type == TOKEN_KEYWORD_IF) {
            consume();
            stmt = parse_if_stmt();
        } else if (tok->type == TOKEN_KEYWORD_UNION) {
            consume();
            stmt = parse_union_decl();
        } else if (tok->type == TOKEN_KEYWORD_SWITCH) {
            consume();
            stmt = parse_switch_stmt();
        } else if (tok->type == TOKEN_KEYWORD_BREAK || tok->type == TOKEN_KEYWORD_CONTINUE) {
            stmt = parse_misc_stmt();
        } else if (tok->type == TOKEN_KEYWORD_TYPEDEF) {
            consume();
            stmt = parse_typedef();
        } else if (tok->type == TOKEN_KEYWORD_ARR) {
            consume();
            stmt = parse_array();
        } else {
            printf("skipped token %zu (%s)\n", pos, token_type_to_string(tok->type));
            consume();
        }

        body = xrealloc(body, sizeof(Node *) * (*out_count + 2));
        body[(*out_count)++] = stmt;
        body[*out_count] = NULL;    
    }
    return body;
}

Node *parse_expression(void) {
    /*
        * Pattern:
        * unnecessary
    */
    Node *expr_node = create_node(NODE_EXPR);
    expr_node->expr.expr = parse_expr();

    expect(TOKEN_SEMICOLON, "Expected semicolon after expression");
    consume();
    return expr_node;
}

Node *parse_enum_decl(void) {
    /*
        * Pattern:
        * enum name {members};
    */
    Node *node = create_node(NODE_ENUM_DECL);

    Token *name = peek();
    if (name && name->type == TOKEN_IDENTIFIER) {
        consume();
        node->enum_decl.name = strdup(name->lexeme);
    } else {
        node->enum_decl.name = NULL; // Anon enum
    }
    
    expect(TOKEN_LBRACE, "Expected '{' after enum name");
    consume();

    node->enum_decl.members = NULL;
    node->enum_decl.member_count = 0;

    while (1) {
        Token *t = peek();
        if (!t) {
            parser_error("Unexpected end of input in enum declaration", t);
            break;
        }

        if (t->type == TOKEN_RBRACE) {
            consume();
            break;
        }

        if (t->type != TOKEN_IDENTIFIER) {
            parser_error("Expected identifier for enum member", t);
            break;
        }

        consume();

        EnumMember member = { strdup(t->lexeme), NULL };

        if (peek() && peek()->type == TOKEN_OPERATOR_ASSIGN) {
            consume();
            member.value = parse_expr();
        }

        node->enum_decl.members = xrealloc(
            node->enum_decl.members,
            sizeof(EnumMember) * (node->enum_decl.member_count + 1)
        );
        node->enum_decl.members[node->enum_decl.member_count++] = member;

        if (peek() && peek()->type == TOKEN_COMMA) {
            consume();
        } else if (peek() && peek()->type == TOKEN_RBRACE) {
            continue;
        } else {
            Token *bad = peek();
            parser_error("Expected ',' or '}' after enum member", bad);
        }
    }
    if (!(node->enum_decl.name) && peek()->type == TOKEN_IDENTIFIER) {
        node->enum_decl.name = strdup(consume()->lexeme);
    }

    expect(TOKEN_SEMICOLON, "Expected ';' after enum declaration");
    consume();
    return node;
}

Node *parse_struct_decl(void) {
    /*
        * Pattern:
        * struct name {members};
    */
    Node *node = create_node(NODE_STRUCT_DECL);

    // name
    Token *name = peek();
    if (name && name->type == TOKEN_IDENTIFIER) {
        consume();
        node->struct_decl.name = strdup(name->lexeme);
    } else {
        node->struct_decl.name = NULL; // Anon struct
    }

    expect(TOKEN_LBRACE, "Expected '{' after struct name");
    consume();

    // members
    Node **members = NULL;
    size_t members_count = 0;

    while (1) {
        Token *t = peek();

        if (!t) {
            parser_error("Unexpected end of input in struct declaration", t);
            break;
        }

        if (t->type == TOKEN_RBRACE) {
            consume();
            break;
        }
        
        Node *member = parse_arg_decl();
        if (!member) {
            parser_error("Expected member", t);
            break;
        }
        members = xrealloc(members, sizeof(Node *) * (members_count + 1));
        members[members_count++] = member;

        expect(TOKEN_SEMICOLON, "Expected semicolon after struct member");
        consume();
    }

    if (!(node->struct_decl.name) && peek()->type == TOKEN_IDENTIFIER) {
        node->struct_decl.name = strdup(consume()->lexeme);
    }

    if (peek()->type == TOKEN_IDENTIFIER) {
        consume();
    }
    expect(TOKEN_SEMICOLON, "Expected ';' after struct declaration");
    consume();

    node->struct_decl.members = members;
    node->struct_decl.member_count = members_count;
    return node;
}

Node *parse_union_decl(void) {
    /*
        * Pattern:
        * union name {members};
    */
    Node *node = create_node(NODE_UNION_DECL);

    // name
    Token *name = peek();
    if (name && name->type == TOKEN_IDENTIFIER) {
        consume();
        node->union_decl.name = strdup(name->lexeme);
    } else {
        node->union_decl.name = NULL; // Anon union
    }

    expect(TOKEN_LBRACE, "Expected '{' after union name");
    consume();

    // members
    Node **members = NULL;
    size_t members_count = 0;

    while (1) {
        Token *t = peek();

        if (!t) {
            parser_error("Unexpected end of input in union declaration", t);
            break;
        }

        if (t->type == TOKEN_RBRACE) {
            consume();
            break;
        }
        
        Node *member = parse_arg_decl();
        if (!member) {
            parser_error("Expected member", t);
            break;
        }
        members = xrealloc(members, sizeof(Node *) * (members_count + 1));
        members[members_count++] = member;

        expect(TOKEN_SEMICOLON, "Expected semicolon after union member");
        consume();
    }

    if (!(node->union_decl.name) && peek()->type == TOKEN_IDENTIFIER) {
        node->union_decl.name = strdup(consume()->lexeme);
    }

    expect(TOKEN_SEMICOLON, "Expected ';' after union declaration");
    consume();

    node->union_decl.members = members;
    node->union_decl.member_count = members_count;
    return node;
}

Node *parse_while_stmt(void) {
    /*
        * Pattern:
        * while (condition) {body}
    */
    Node *node = create_node(NODE_WHILE_STMT);

    expect(TOKEN_LPAREN, "Expected '(' after while");
    consume();
    node->while_stmt.cond = parse_expr();

    expect(TOKEN_RPAREN, "Expected ')' after condition");
    consume();

    expect(TOKEN_LBRACE, "Expected '{' after condition");
    consume();

    size_t body_count = 0;
    node->while_stmt.body = parse_block(&body_count);
    if (!node->while_stmt.body) {
        node->while_stmt.body = xcalloc(1, sizeof(Node *));
    }

    expect(TOKEN_RBRACE, "Expected '}' after while body");
    consume();
    return node;
}

Node *parse_do_while_stmt(void) {
    /*
        * Pattern:
        * do {body} while (condition);
    */
    Node *node = create_node(NODE_DO_WHILE_STMT);
    
    // body
    expect(TOKEN_LBRACE, "Expected '{' after do");
    consume();

    size_t body_count = 0;
    node->do_while_stmt.body = parse_block(&body_count);
    if (!node->do_while_stmt.body) {
        node->do_while_stmt.body = xcalloc(1, sizeof(Node *));
    }

    expect(TOKEN_RBRACE, "Expected '}' after do body");
    consume();

    // while
    expect(TOKEN_KEYWORD_WHILE, "Expected while after do body");
    consume();

    // condition
    expect(TOKEN_LPAREN, "Expected '(' after while");
    consume();
    node->do_while_stmt.cond = parse_expr();

    expect(TOKEN_RPAREN, "Expected ')' after condition");
    consume();

    // semicolon at end
    expect(TOKEN_SEMICOLON, "Expected ';' after condition");
    consume();
    return node;
}

Node *parse_for_stmt(void) {
    /*
        * Pattern:
        * for ([inc];[cond];[inc]) {body}
    */
    Node *node = create_node(NODE_FOR_STMT);
    node->for_stmt.init = NULL;
    node->for_stmt.cond = NULL;
    node->for_stmt.inc = NULL;
    node->for_stmt.body = NULL;

    // initialiser
    expect(TOKEN_LPAREN, "Expected '(' after for");
    consume();

    Token *tok_init = peek();
    if (tok_init->type != TOKEN_SEMICOLON) {
        node->for_stmt.init = parse_var_decl(); // consumes semicolon, var keyword not alowed
    } else {
        node->for_stmt.init = NULL;
        consume();
    }

    // condition
    Token *tok_cond = peek();
    if (tok_cond->type != TOKEN_SEMICOLON) {
        node->for_stmt.cond = parse_expression(); // consumes semicolon
    } else {
        node->for_stmt.cond = NULL;
        consume();

    }

    // increment
    Token *tok_inc = peek();
    if (tok_inc->type != TOKEN_RPAREN) {
        node->for_stmt.inc = parse_expr(); // no semicolon
    } else {
        node->for_stmt.inc = NULL;
    }
    expect(TOKEN_RPAREN, "Expected ')' after condition");
    consume();

    expect(TOKEN_LBRACE, "Expected '{' after condition");
    consume();

    size_t body_count = 0;
    node->for_stmt.body = parse_block(&body_count);
    if (!node->for_stmt.body) {
        node->for_stmt.body = xcalloc(1, sizeof(Node *));
    }

    expect(TOKEN_RBRACE, "Expected '}' after for body");
    consume();
    return node;
}

TypeSpec *parse_type_spec(void) {
    TypeSpec *spec = xcalloc(1, sizeof(TypeSpec));

    while (is_property(peek()->type)) {
        Token *t = consume();
        switch (t->type) {
            case TOKEN_KEYWORD_AUTO:     spec->storage = AUTO; break;
            case TOKEN_KEYWORD_REGISTER: spec->storage = REGISTER; break;
            case TOKEN_KEYWORD_STATIC:   spec->storage = STATIC; break;
            case TOKEN_KEYWORD_EXTERN:   spec->storage = EXTERN; break;
            case TOKEN_KEYWORD_SIGNED:   spec->sign = SIGNED_T; break;
            case TOKEN_KEYWORD_UNSIGNED: spec->sign = UNSIGNED_T; break;
            case TOKEN_KEYWORD_SHORT:    spec->length = SHORT_T; break;
            case TOKEN_KEYWORD_LONG:
                if (spec->length == LONG_T) {
                    spec->length = LONGLONG_T;
                } else {
                    spec->length = LONG_T;
                }
                break;
            case TOKEN_KEYWORD_CONST:    spec->is_const = 1; break;
            case TOKEN_KEYWORD_VOLATILE: spec->is_volatile = 1; break;
            case TOKEN_KEYWORD_INLINE:   spec->is_inline = 1; break;
            case TOKEN_KEYWORD_RESTRICT: spec->is_restrict = 1; break;
            default: break;
        }
    }

    return spec;
}

Node *parse_type(void) {
    Node *node = create_node(NODE_TYPE);

    node->type_node.spec = parse_type_spec();

    node->type_node.base = NULL;
    node->type_node.decl = NULL;
    node->type_node.is_decl = 0;

    if (is_type(peek()->type)) {
        node->type_node.base = strdup(peek()->lexeme);
        node->type_node.is_decl = 0;
        consume();
    } else if (peek()->type == TOKEN_IDENTIFIER && peek_next()->type == TOKEN_IDENTIFIER) {
        node->type_node.base = strdup(peek()->lexeme);
        node->type_node.is_decl = 0;
        consume();
    } else if (peek()->type == TOKEN_KEYWORD_STRUCT) {
        node->type_node.decl = parse_struct_decl();
        node->type_node.is_decl = 1;
    } else if (peek()->type == TOKEN_KEYWORD_ENUM) {
        node->type_node.decl = parse_enum_decl();
        node->type_node.is_decl = 1;
    } else if (peek()->type == TOKEN_KEYWORD_UNION) {
        node->type_node.decl = parse_union_decl();
        node->type_node.is_decl = 1;
    } else {
        /* Default to int if only specifiers were given (e.g., "long", "unsigned") */
        node->type_node.base = strdup("int");
        node->type_node.is_decl = 0;
    }

    while (peek()->type == TOKEN_OPERATOR_STAR) {
        node->type_node.spec->pointer_depth++;
        consume();
    }

    return node;
}

Node *parse_if_stmt(void) {
    /*
        * Pattern:
        * if (cond) {body} [if else|else {body}]
    */
    Node *node = create_node(NODE_IF_STMT);

    node->if_stmt.if_cond = NULL;
    node->if_stmt.if_body = NULL;
    node->if_stmt.elif_conds = NULL;
    node->if_stmt.elif_bodies = NULL;
    node->if_stmt.elif_count = 0;
    node->if_stmt.else_body = NULL;

    expect(TOKEN_LPAREN, "Expected '(' after if keyword");
    consume();
    node->if_stmt.if_cond = parse_expr();
    expect(TOKEN_RPAREN, "Expected ')' after if condition");
    consume();

    expect(TOKEN_LBRACE, "Expected '{' after if condition");
    consume();
    size_t body_count = 0;
    node->if_stmt.if_body = parse_block(&body_count);
    if (!node->if_stmt.if_body) {
        node->if_stmt.if_body = xcalloc(1, sizeof(Node *));
    }
    expect(TOKEN_RBRACE, "Expected '}' after if body");
    consume();

    while (peek() && peek()->type == TOKEN_KEYWORD_ELSE && peek_next() && peek_next()->type == TOKEN_KEYWORD_IF) {
        consume();
        consume();

        expect(TOKEN_LPAREN, "Expected '(' after 'elif'");
        consume();
        Expr *elif_cond = parse_expr();
        expect(TOKEN_RPAREN, "Expected ')' after elif condition");
        consume();

        expect(TOKEN_LBRACE, "Expected '{' after elif condition");
        consume();
        size_t elif_body_count = 0;
        Node **elif_body = parse_block(&elif_body_count);
        if (!elif_body) {
            elif_body = xcalloc(1, sizeof(Node *));
        }
        expect(TOKEN_RBRACE, "Expected '}' after elif body");
        consume();

        // Add to lists
        node->if_stmt.elif_conds = xrealloc(node->if_stmt.elif_conds, sizeof(Expr*) * (node->if_stmt.elif_count + 1));
        node->if_stmt.elif_bodies = xrealloc(node->if_stmt.elif_bodies, sizeof(Node**) * (node->if_stmt.elif_count + 1));

        node->if_stmt.elif_conds[node->if_stmt.elif_count] = elif_cond;
        node->if_stmt.elif_bodies[node->if_stmt.elif_count] = elif_body;
        node->if_stmt.elif_count++;
    }

    if (peek() && peek()->type == TOKEN_KEYWORD_ELSE) {
        consume();
        expect(TOKEN_LBRACE, "Expected '{' after else");
        consume();
        size_t else_body_count = 0;
        Node **else_body = parse_block(&else_body_count);
        if (!else_body) {
            else_body = xcalloc(1, sizeof(Node*)); // always null-terminated
        }
        node->if_stmt.else_body = else_body;
        expect(TOKEN_RBRACE, "Expected '}' after else body");
        consume();
    }

    return node;
}

Node *parse_switch_stmt(void) {
    Node *node = create_node(NODE_SWITCH_STMT);

    node->switch_stmt.cases        = NULL;
    node->switch_stmt.case_bodies  = NULL;
    node->switch_stmt.case_count   = 0;
    node->switch_stmt.default_body = NULL;

    expect(TOKEN_LPAREN, "Expected '(' after switch keyword");
    consume();

    node->switch_stmt.expression = parse_expr();

    expect(TOKEN_RPAREN, "Expected ')' after switch expression");
    consume();

    expect(TOKEN_LBRACE, "Expected '{' after switch expression");
    consume();

    size_t capacity = 4;
    node->switch_stmt.cases       = xmalloc(sizeof(struct Expr *) * capacity);
    node->switch_stmt.case_bodies = xmalloc(sizeof(struct Node **) * capacity);

    while (peek()->type != TOKEN_RBRACE &&peek()->type != TOKEN_EOF) {
        Token *t = peek();

        if (t->type == TOKEN_KEYWORD_CASE) {
            consume();

            expect(TOKEN_LPAREN, "Expected '(' after case");
            consume();

            Expr *case_expr = parse_expr();

            expect(TOKEN_RPAREN, "Expected ')' after case expression");
            consume();

            expect(TOKEN_LBRACE, "Expected '{' to start case body");
            consume();

            size_t stmt_count = 0;
            Node **stmts = parse_block(&stmt_count);

            expect(TOKEN_RBRACE, "Expected '}' after case body");
            consume();

            // ensure capacity
            if (node->switch_stmt.case_count >= capacity) {
                capacity *= 2;

                node->switch_stmt.cases =
                    xrealloc(node->switch_stmt.cases,
                             sizeof(struct Expr *) * capacity);

                node->switch_stmt.case_bodies =
                    xrealloc(node->switch_stmt.case_bodies,
                             sizeof(struct Node **) * capacity);
            }

            size_t idx = node->switch_stmt.case_count++;
            node->switch_stmt.cases[idx] = case_expr;
            node->switch_stmt.case_bodies[idx] = stmts;
            continue;
        }

        if (t->type == TOKEN_KEYWORD_DEFAULT) {
            consume();

            expect(TOKEN_LBRACE, "Expected '{' after default");
            consume();

            size_t stmt_count = 0;
            node->switch_stmt.default_body = parse_block(&stmt_count);

            expect(TOKEN_RBRACE, "Expected '}' after default body");
            consume();

            continue;
        }

        parser_error("Expected 'case' or 'default' inside switch", t);
        break;
    }

    expect(TOKEN_RBRACE, "Expected '}' after switch body");
    consume();

    return node;
}

Node *parse_misc_stmt(void) {
    Node *node = create_node(NODE_MISC);

    Token *tok = peek();
    if (tok->type != TOKEN_KEYWORD_BREAK && tok->type != TOKEN_KEYWORD_CONTINUE) {
        parser_error("Expected 'break' or 'continue'", tok);
        return NULL;
    }
    node->misc.name = strdup(tok->lexeme);
    consume();

    expect(TOKEN_SEMICOLON, "Expected semicolon after statement");
    consume();
    return node;
}

Node *parse_typedef(void) {
    /*
        * Pattern:
        * typedef [type | struct | union | enum] name;
    */
    Node *node = create_node(NODE_TYPEDEF);

    Token *t = peek();
    if (t->type == TOKEN_KEYWORD_STRUCT) {
        consume();
        Node *type = parse_struct_decl();
        node->typedef_node.type = type;
        node->typedef_node.name = strdup(type->struct_decl.name);
    } else if (t->type == TOKEN_KEYWORD_UNION) {
        consume();
        Node *type = parse_union_decl();
        node->typedef_node.type = type;
        node->typedef_node.name = strdup(type->union_decl.name);
    } else if (t->type == TOKEN_KEYWORD_ENUM) {
        consume();
        Node *type = parse_enum_decl();
        node->typedef_node.type = type;
        node->typedef_node.name = strdup(type->enum_decl.name);
    } else {
        /* Parse: type name; */
        Node *arg_decl = parse_arg_decl();
        node->typedef_node.type = arg_decl->var_decl.type;
        node->typedef_node.name = strdup(arg_decl->var_decl.name);
        arg_decl->var_decl.type = NULL;
        expect(TOKEN_SEMICOLON, "expected ';' after typedef");
        consume();
    }
    return node;
}

Node *parse_array(void) {
    /*
        * Pattern:
        * arr<[properties] type> name\[dimensions\] [assignment value];
    */
    Node *node = create_node(NODE_ARRAY);
    node->array.dim = NULL;
    node->array.dim_count = 0;
    node->array.name = NULL;
    node->array.type = NULL;
    node->array.value = NULL;

    expect(TOKEN_OPERATOR_LOWER, "expected '<' after arr keyword");
    consume();
    node->array.type = parse_type();
    expect(TOKEN_OPERATOR_GREATER, "expected '>' after array type");
    consume();

    expect(TOKEN_IDENTIFIER, "expected array name after arr");
    node->array.name = strdup(peek()->lexeme);
    consume();

    while (peek()->type == TOKEN_LBRACKET) {
        consume();

        expect(TOKEN_LITERAL_INT, "Expected array dimension size");
        Token *size_tok = consume();
        size_t size = (size_t)atoi(size_tok->lexeme);
        
        expect(TOKEN_RBRACKET, "Expected ']' after dimension");
        consume();
        
        node->array.dim = xrealloc(node->array.dim, sizeof(size_t) * (node->array.dim_count + 1));
        node->array.dim[node->array.dim_count++] = size;
    }

    if (peek()->type == TOKEN_OPERATOR_ASSIGN) {
        consume();  // '='
        node->array.value = parse_expr();
    }

    expect(TOKEN_SEMICOLON, "expected ';' after array declaration");
    consume();
    return node;
}