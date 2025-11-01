/*
	* parser.c - Owly Parser
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
#include "parser.h"
#include "memutils.h"
#include "owlylexer.h"
#include "ast.h"
#include "ast_to_json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
/* --- Globals ---*/
Token token_list[MAX_TOKENS];
size_t token_count = 0;
size_t pos = 0;

extern int debug;
/* --- Protypes --- */
void parser_error(const char *msg, Token *t);
static Token *peek(void);
static Token *peek_next(void);
static Token *consume(void);
static int match(TokenType type, const char *lexeme);
static int is_at_end(void);
int is_property(TokenType type);
int is_type(TokenType type);
int is_literal(TokenType type);
void validate_specifiers(const char **props);
TokenType type_from_string(const char *str);
void parser_init(void);
void parse_program(void);
Node *parse_var_decl(void);
Node *parse_func_decl(void);
Node *parse_arg_decl(void);
Node *parse_ret(void);
Node **parse_block(size_t *out_count);

/* --- Functions --- */
void parser_error(const char *msg, Token *t) {
    if (t) {
        fprintf(stderr, "Parser error: %s at token '%.*s'\n", msg, (int)t->length, t->lexeme);
        exit(1);
    } else {
        fprintf(stderr, "Parser error: %s\n", msg);
        exit(1);
    }
}

static Token *peek(void) {
    if (pos < token_count) return &token_list[pos];
    return NULL;
}

static Token *peek_next(void) {
    if (pos + 1 < token_count) return &token_list[pos + 1];
    return NULL;
}

static Token *consume(void) {
    if (pos < token_count) return &token_list[pos++];
    return NULL;
}

static int match(TokenType type, const char *lexeme) {
    Token *t = peek();
    if (t && t->type == type && (!lexeme || 
        (strlen(lexeme) == t->length && strncmp(t->lexeme, lexeme, t->length) == 0))) {
        consume();
        return 1;
    }
    return 0;
}

static int is_at_end(void) {
    return pos >= token_count || token_list[pos].type == TOKEN_EOF;
}

int is_property(TokenType type) {
    switch (type) {
        case TOKEN_KEYWORD_AUTO:
        case TOKEN_KEYWORD_REGISTER:
        case TOKEN_KEYWORD_CONST:
        case TOKEN_KEYWORD_EXTERN:
        case TOKEN_KEYWORD_INLINE:
        case TOKEN_KEYWORD_LONG:
        case TOKEN_KEYWORD_RESTRICT:
        case TOKEN_KEYWORD_SHORT:
        case TOKEN_KEYWORD_SIGNED:
        case TOKEN_KEYWORD_STATIC:
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
    else if (strncmp(str, "BREAK", sizeof("BREAK")) == 0) return TOKEN_KEYWORD_BREAK;
    else if (strncmp(str, "CASE", sizeof("CASE")) == 0)  return TOKEN_KEYWORD_CASE;
    else if (strncmp(str, "CHAR", sizeof("CHAR")) == 0) return TOKEN_KEYWORD_CHAR;
    else if (strncmp(str, "CONST", sizeof("CONST")) == 0) return TOKEN_KEYWORD_CONST;
    else if (strncmp(str, "CONTINUE", sizeof("CONTINUE")) == 0) return TOKEN_KEYWORD_CONTINUE;
    else if (strncmp(str, "DEFAULT", sizeof("DEFAULT")) == 0) return TOKEN_KEYWORD_DEFAULT;
    else if (strncmp(str, "DO", sizeof("DO") == 0)) return TOKEN_KEYWORD_DO;
    else if (strncmp(str, "DOUBLE", sizeof("DOUBLE")) == 0) return TOKEN_KEYWORD_DOUBLE;
    else if (strncmp(str, "ELSE", sizeof("ELSE")) == 0) return TOKEN_KEYWORD_ELSE;
    else if (strncmp(str, "ENUM", sizeof("ENUM")) == 0) return TOKEN_KEYWORD_ENUM;
    else if (strncmp(str, "EXTERN", sizeof("EXTERN")) == 0) return TOKEN_KEYWORD_EXTERN;
    else if (strncmp(str, "FLOAT", sizeof("FLOAT")) == 0) return TOKEN_KEYWORD_FLOAT;
    else if (strncmp(str, "FOR", sizeof("FOR")) == 0) return TOKEN_KEYWORD_FOR;
    else if (strncmp(str, "FUNC", sizeof("FUNC")) == 0) return TOKEN_KEYWORD_FUNC;
    else if (strncmp(str, "GOTO", sizeof("GOTO")) == 0) return TOKEN_KEYWORD_GOTO;
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
    else if (strncmp(str, "WHILE", sizeof("WHILE") == 0)) return TOKEN_KEYWORD_WHILE;
    else if (strncmp(str, "_BOOL", sizeof("_BOOL") == 0)) return TOKEN_KEYWORD_BOOL;
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
}

void parser_init(void) {
    char type_str[64];
    char buf[LINEMAX];
    FILE *tokens = fopen("list.tok", "r");
    while (fscanf(tokens, "%63[^,], \"%[^\"]\";\n", type_str, buf) == 2) {
        Token tok;
        tok.type = type_from_string(type_str);
        tok.lexeme = strdup(buf);
        tok.length = (size_t)strlen(buf);
        token_list[token_count++] = tok;
    }
    fclose(tokens);

    parse_program();
    
}

void parse_program(void) {
    cJSON *root = cJSON_CreateObject();
    cJSON *program = cJSON_AddArrayToObject(root, "PROGRAM");  

    size_t stmt_count = 0;
    Node **stmts = parse_block(&stmt_count);

    for (size_t i = 0; i < stmt_count; i++) {
        cJSON *item = NULL;
        switch (stmts[i]->type) {
            case NODE_VAR_DECL:
                item = var_decl_to_json(stmts[i]);
                break;
            case NODE_FUNC_DECL:
                item = func_decl_to_json(stmts[i]);
                break;
            default:
                break;
        }
        if (item) {
            cJSON_AddItemToArray(program, item);
        }
    }
    char *json_str = cJSON_Print(root);
    FILE *json_file = fopen("ast.json", "w");
    if (json_file) {
        fputs(json_str, json_file);
        fclose(json_file);
    }
    if (debug) {
        printf("%s\n", json_str);
    }

    cJSON_free(json_str);
    cJSON_Delete(root);
}

Node *parse_var_decl(void) {
    /*
        * Pattern:
        * var [properties] type [*]ident [= literal|identifier|&identifier]
    */
    Node *node = create_node(NODE_VAR_DECL);
    // Properties
    int properties = 0;
    while (is_property(peek()->type)) {
        if (properties >= 11) {
            parser_error("too many properties, meaning you have at least one twice", peek());
        }
        Token *t = peek();
        node->var_decl.properties[properties++] = strdup(token_type_to_string(t->type));
        validate_specifiers(node->var_decl.properties);
        consume();
    }
    node->var_decl.properties[properties] = NULL;
    // Type
    if (!is_type(peek()->type)) {
        printf("[WARN]: No type specified for variable '%s', defaulting to int\n", peek()->lexeme);
        node->var_decl.type = strdup("int");
    } else {
        Token *type = consume();
        node->var_decl.type = strdup(type->lexeme);
    }
    node->var_decl.is_pointer = 0;
    if (peek()->type == TOKEN_OPERATOR_STAR) {
        node->var_decl.is_pointer = 1;
        consume();
    }
    // Name
    if (peek()->type != TOKEN_IDENTIFIER) {
        parser_error("Expected variable name after type", peek());
    }
    Token *name = consume();
    node->var_decl.name = strdup(name->lexeme);

    if (peek()->type != TOKEN_SEMICOLON) {
        if (peek()->type != TOKEN_OPERATOR_ASSIGN) {
            parser_error("need assign to initialize data", peek());
        }
        consume();
        Token *val = consume();
        if (is_literal(val->type)) {
            node->var_decl.value = strdup(val->lexeme);
        } else if (val->type == TOKEN_IDENTIFIER) {
            node->var_decl.value = strdup(val->lexeme);
        } else if (val->type == TOKEN_OPERATOR_AMP) {
            Token *target = consume();
            if (target->type != TOKEN_IDENTIFIER) {
                parser_error("Expedted identifier after &", target);
            }
            size_t len = 1 + strlen(target->lexeme) + 1;
            char *dest = xmalloc(len);
            snprintf(dest, len, "&%s", target->lexeme);
            node->var_decl.value = dest;
        }
    }
    Token *tok = consume();
    if (tok->type != TOKEN_SEMICOLON) {
        parser_error("Expected semicolon", tok);
    }
    printf("\n[VAR DECL] %s%s %s = %s\n",
        node->var_decl.type,
        node->var_decl.is_pointer ? "*" : "",
        node->var_decl.name,
        node->var_decl.value ? node->var_decl.value : "<uninitialized>");
    return node;
}

Node *parse_func_decl(void) {
    /*
        * Pattern:
        * func [properties] type [*]ident([void| typeargs,]) {[body]}
    */

    Node *node = create_node(NODE_FUNC_DECL);
    // Properties
    int properties = 0;

    while (is_property(peek()->type)) {
        if (properties >= 11) {
            parser_error("too many properties, meaning you have at least one twice", peek());
        }
        Token *t = peek();
        node->func_decl.properties[properties++] = strdup(token_type_to_string(t->type));
        validate_specifiers(node->func_decl.properties);
        consume();
    }
    node->func_decl.properties[properties] = NULL;

    // Type
    if (!is_type(peek()->type)) {
        printf("[WARN]: No type specified for variable '%s', defaulting to int\n", peek()->lexeme);
        node->func_decl.type = strdup("int");
    } else {
        Token *type = consume();
        node->func_decl.type = strdup(type->lexeme);
    }

    node->func_decl.is_pointer = 0;
    if (peek()->type == TOKEN_OPERATOR_STAR) {
        node->func_decl.is_pointer = 1;
        consume();
    }

    // Name
    if (peek()->type != TOKEN_IDENTIFIER) {
        parser_error("Expected function name after type", peek());
    }
    Token *name = consume();
    node->func_decl.name = strdup(name->lexeme);

    Token *lparen = consume();
    if (lparen->type != TOKEN_LPAREN) {
        parser_error("expected opening parenthesis after function name", lparen);
    }

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
    Token *rparen = consume();
    if (rparen->type != TOKEN_RPAREN) {
        parser_error("Expected closing parenthesis", rparen);
    }
    if (peek()->type == TOKEN_SEMICOLON) {
        // Prototype
        node->func_decl.is_prototype = 1;
        node->func_decl.body = NULL;
        node->func_decl.body_count = 0;
        consume();
        printf("\n[FUNC DECL] %s%s %s(",
            node->func_decl.type,
            node->func_decl.is_pointer ? "*" : "",
            node->func_decl.name
        );
        
        // Print function arguments
        for (size_t i = 0; i < node->func_decl.arg_count; i++) {
            Node *arg = node->func_decl.args[i];
            printf("%s%s %s",
                arg->var_decl.type,
                arg->var_decl.is_pointer ? "*" : "",
                arg->var_decl.name
            );
            if (i + 1 < node->func_decl.arg_count) {
                printf(", ");
            }
        }
        
        printf(")");
        
        if (node->func_decl.is_prototype) {
            printf(";\n");  // Prototype ends with semicolon
        } else {
            printf(" { ... }\n");  // Body exists
        }
        
        return node;
    }

    Token *lbrace = consume();
    if (lbrace->type != TOKEN_LBRACE) {
        parser_error("expected '{' before function body", lbrace);
    }
    node->func_decl.body = parse_block(&node->func_decl.body_count);

    Token *rbrace = consume();
    if (rbrace->type != TOKEN_RBRACE) {
        parser_error("expected '}' after function body", rbrace);
    }
    printf("\n[FUNC DECL] %s%s %s(",
        node->func_decl.type,
        node->func_decl.is_pointer ? "*" : "",
        node->func_decl.name
    );
    
    // Print function arguments
    for (size_t i = 0; i < node->func_decl.arg_count; i++) {
        Node *arg = node->func_decl.args[i];
        printf("%s%s %s",
            arg->var_decl.type,
            arg->var_decl.is_pointer ? "*" : "",
            arg->var_decl.name
        );
        if (i + 1 < node->func_decl.arg_count) {
            printf(", ");
        }
    }
    
    printf(")");
    
    if (node->func_decl.is_prototype) {
        printf(";\n");  // Prototype ends with semicolon
    } else {
        printf(" { ... }\n");  // Body exists
    }
    
    return node;
}

Node *parse_arg_decl(void) {
    /*
        * Pattern:
        * [properties] type [*]ident
    */
    Node *node = create_node(NODE_VAR_DECL);
    // Properties
    int properties = 0;
    while (is_property(peek()->type)) {
        if (properties >= 11) {
            parser_error("too many properties, meaning you have at least one twice", peek());
        }
        Token *t = peek();
        node->var_decl.properties[properties++] = strdup(token_type_to_string(t->type));
        validate_specifiers(node->var_decl.properties);
        consume();
    }
    node->var_decl.properties[properties] = NULL;
    
    // Type
    if (!is_type(peek()->type)) {
        printf("[WARN]: No type specified for variable '%s', defaulting to int\n", peek()->lexeme);
        node->var_decl.type = strdup("int");
    } else {
        Token *type = consume();
        node->var_decl.type = strdup(type->lexeme);
    }
    node->var_decl.is_pointer = 0;
    if (peek()->type == TOKEN_OPERATOR_STAR) {
        node->var_decl.is_pointer = 1;
        consume();
    }
    // Name
    if (peek()->type != TOKEN_IDENTIFIER) {
        parser_error("Expected variable name after type", peek());
    }
    
    Token *name = consume();
    node->var_decl.name = strdup(name->lexeme);
    return node;
}

Node *parse_ret(void) {
    Token *return_tok = consume();

    Node *node = create_node(NODE_RETURN);

    if (peek()->type != TOKEN_SEMICOLON) {
        Token *val = consume();
        node->return_stmt.value = strdup(val->lexeme);
    } else {
        node->return_stmt.value = NULL;
    }
    Token *semicolon = consume();
    if (semicolon->type != TOKEN_SEMICOLON) {
        parser_error("Expected ';' after return statement", semicolon);
    }
    return node;
}

Node **parse_block(size_t *out_count) {
    Node **body = NULL;
    *out_count = 0;

    while (1) {
        Token *tok = peek();
        if (!tok || tok->type == TOKEN_EOF || tok->type == TOKEN_RBRACE) {
            break;
        }
        if (tok->type == TOKEN_KEYWORD_VAR) {
            consume();
            Node *var_decl = parse_var_decl();
            body = xrealloc(body, sizeof(Node *) * (*out_count + 1));
            body[(*out_count)++] = var_decl;
        } else if (tok->type == TOKEN_KEYWORD_FUNC) {
            consume();
            Node *func_decl = parse_func_decl();
            body = xrealloc(body, sizeof(Node *) * (*out_count + 1));
            body[(*out_count)++] = func_decl;
        } else if (tok->type == TOKEN_KEYWORD_RETURN) {
            Node *ret_stmt = parse_ret();
            body = xrealloc(body, sizeof(Node *) * (*out_count + 1));
            body[(*out_count)++] = ret_stmt;
        } else {
            printf("skipped token %zu (%s)\n", pos, token_type_to_string(tok->type));
            consume();
        }
    }
    return body;
}