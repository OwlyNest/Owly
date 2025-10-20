
#include "parser.h"
#include "memutils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parser_error(const char *msg, Token *t) {
    if (t) {
        fprintf(stderr, "Parser error: %s at token '%.*s'\n", msg, (int)t->length, t->lexeme);
    } else {
        fprintf(stderr, "Parser error: %s\n", msg);
    }
}

// Helpers to peek and consume tokens
static Token *peek(Parser *p) {
    if (p->pos < p->count) return &p->tokens[p->pos];
    return NULL;
}

static Token *consume(Parser *p) {
    if (p->pos < p->count) return &p->tokens[p->pos++];
    return NULL;
}

static int match(Parser *p, TokenType type, const char *lexeme) {
    Token *t = peek(p);
    if (t && t->type == type && (!lexeme || 
        (strlen(lexeme) == t->length && strncmp(t->lexeme, lexeme, t->length) == 0))) {
        consume(p);
        return 1;
    }
    return 0;
}

// Forward declarations
static ASTNode *parse_function(Parser *p);
static ASTNode *parse_print_statement(Parser *p);
static ASTNode *parse_statement(Parser *p);
static ASTNode *parse_if_statement(Parser *p);
static ASTNode *parse_for_statement(Parser *p);         
static ASTNode *parse_while_statement(Parser *p);       
static ASTNode *parse_variable_declaration(Parser *p);
static ASTNode *parse_expression(Parser *p);

void parser_init(Parser *parser, Token *tokens, size_t count) {
    parser->tokens = tokens;
    parser->count = count;
    parser->pos = 0;
}

ASTNode *parse_program(Parser *p) {
    ASTNode *program = ast_new_program();

    while (p->pos < p->count) {
        Token *t = peek(p);
        if (!t || t->type == TOKEN_EOF) break;

        if (t->type == TOKEN_KEYWORD_FUNC) {
            // Let parse_function() consume the 'func' token
            ASTNode *func = parse_function(p);
            if (!func) {
                parser_error("Failed to parse function", peek(p));
                ast_free(program);
                return NULL;
            }
            ast_add_statement(program, func);
        } else {
            parser_error("Unexpected token", t);
            ast_free(program);
            return NULL;
        }
    }

    return program;
}

static ASTNode *parse_function(Parser *p) {
    if (!match(p, TOKEN_KEYWORD_FUNC, "func")) {
        parser_error("Expected 'func' keyword", peek(p));
        return NULL;
    }

    Token *name_token = peek(p);
    if (!name_token || name_token->type != TOKEN_IDENTIFIER) {
        parser_error("Expected function name after 'func'", name_token);
        return NULL;
    }
    char *func_name = strndup(name_token->lexeme, name_token->length);
    consume(p);

    if (!match(p, TOKEN_SYMBOL, "(")) {
        parser_error("Expected '(' after function name", peek(p));
        free(func_name);
        return NULL;
    }

    if (!match(p, TOKEN_SYMBOL, ")")) {
        parser_error("Expected ')' after '('\n", peek(p));
        free(func_name);
        return NULL;
    }

    if (!match(p, TOKEN_SYMBOL, "{")) {
        parser_error("Expected '{' to start function body", peek(p));
        free(func_name);
        return NULL;
    }

    ASTNode *func_node = ast_new_function(func_name);
    free(func_name);

    // Parse statements inside function body
    while (1) {
        Token *t = peek(p);
        if (!t) {
            fprintf(stderr, "Unexpected EOF in function body\n");
            ast_free(func_node);
            return NULL;
        }
        if (t->type == TOKEN_SYMBOL && t->length == 1 && strncmp(t->lexeme, "}", 1) == 0) {
            consume(p); // consume '}'
            break;  
        }
        ASTNode *stmt = parse_statement(p);
        if (!stmt) {
            fprintf(stderr, "Failed to parse statement in function body\n");
            ast_free(func_node);
            return NULL;
        }
        ast_add_statement(func_node, stmt);
    }
    return func_node;
}

static ASTNode *parse_print_statement(Parser *p) {
    if (!match(p, TOKEN_KEYWORD_PRINT, "print")) {
        fprintf(stderr, "Expected 'print'\n");
        return NULL;
    }

    if (!match(p, TOKEN_SYMBOL, "(")) {
        fprintf(stderr, "Expected '(' after 'print'\n");
        return NULL;
    }

    Token *str_token = peek(p);
    if (!str_token || (str_token->type != TOKEN_STRING && str_token->type != TOKEN_NUMBER && str_token->type != TOKEN_IDENTIFIER)) {
        fprintf(stderr, "Expected value inside print()\n");
        return NULL;
    }
    char *str_value = strndup(str_token->lexeme, str_token->length);
    consume(p);

    if (!match(p, TOKEN_SYMBOL, ")")) {
        fprintf(stderr, "Expected ')' after print argument\n");
        free(str_value);
        return NULL;
    }

    ASTNode *str_node = ast_new_literal_string(str_value);
    free(str_value);

    return ast_new_print(str_node);
}

static ASTNode *parse_statement(Parser *p) {
    Token *t = peek(p);
    if (!t) return NULL;

    switch (t->type) {
        case TOKEN_KEYWORD_PRINT:
            return parse_print_statement(p);
        case TOKEN_IF:
            return parse_if_statement(p);
        case TOKEN_FOR:
            return parse_for_statement(p);
        case TOKEN_WHILE:
            return parse_while_statement(p);
        case TOKEN_KEYWORD_STR:
        case TOKEN_KEYWORD_INT:
        case TOKEN_KEYWORD_DOUBLE:
        case TOKEN_KEYWORD_LIST:
        case TOKEN_KEYWORD_ARRAY:
            return parse_variable_declaration(p);
        //new stuf goes here
        default:
            // Try parsing as an expression statement
            ASTNode *expr = parse_expression(p);
            if (expr) {
                return expr;
            }
            parser_error("Unknown statement starting with token", t);
            return NULL;
    }
}

static ASTNode *parse_if_statement(Parser *p) {
    if (!match(p, TOKEN_IF, "if")) {
        fprintf(stderr, "Expected 'if' keyword\n");
        return NULL;
    }
    if (!match(p, TOKEN_SYMBOL, "(")) {
        fprintf(stderr, "Expected '(' after 'if'\n");
        return NULL;
    }
    ASTNode *cond_node = parse_expression(p);
    if (!cond_node) {
        parser_error("Expected condition expression in if statement", peek(p));
        return NULL;
    }
    if (!match(p, TOKEN_SYMBOL, ")")) {
        fprintf(stderr, "Expected ')' after if condition\n");
        return NULL;
    }
    if (!match(p, TOKEN_SYMBOL, "{")) {
        fprintf(stderr, "Expected '{' to start if body\n");
        return NULL;
    }

    ASTNode *if_node = ast_new_if(cond_node);

    // Parse statements inside if body
    while (1) {
        Token *t = peek(p);
        if (!t) {
            fprintf(stderr, "Unexpected EOF in if body\n");
            ast_free(if_node);
            return NULL;
        }
        // Check for closing brace
        if (t->type == TOKEN_SYMBOL && t->length == 1 && strncmp(t->lexeme, "}", 1) == 0) {
            consume(p); // consume '}'
            break;
        }
        ASTNode *stmt = parse_statement(p);
        if (!stmt) {
            fprintf(stderr, "Failed to parse statement in if body\n");
            ast_free(if_node);
            return NULL;
        }
        ast_add_statement(if_node, stmt);
    }

    // Now parse zero or more elif blocks
    while (1) {
        Token *t = peek(p);
        if (!t || t->type != TOKEN_ELIF) break;

        consume(p); // consume 'elif'
        if (!match(p, TOKEN_SYMBOL, "(")) {
            fprintf(stderr, "Expected '(' after 'elif'\n");
            return NULL;
        }
        ASTNode *cond_node = parse_expression(p);
        if (!cond_node) {
            parser_error("Expected condition expression in elif statement", peek(p));
            return NULL;
        }
        if (!match(p, TOKEN_SYMBOL, ")")) {
            fprintf(stderr, "Expected ')' after elif condition\n");
            return NULL;
        }
        if (!match(p, TOKEN_SYMBOL, "{")) {
            fprintf(stderr, "Expected '{' to start elif body\n");
            return NULL;
        }

        // Parse elif body
        ASTNode **elif_body = NULL;
        size_t elif_body_count = 0;
        while (1) {
            Token *et = peek(p);
            if (!et) {
                fprintf(stderr, "Unexpected EOF in elif body\n");
                ast_free(if_node);
                return NULL;
            }
            if (et->type == TOKEN_SYMBOL && et->length == 1 && strncmp(et->lexeme, "}", 1) == 0) {
                consume(p);
                break;
            }
            ASTNode *stmt = parse_statement(p);
            if (!stmt) {
                fprintf(stderr, "Failed to parse statement in elif body\n");
                ast_free(if_node);
                return NULL;
            }
            elif_body = realloc(elif_body, sizeof(ASTNode*) * (elif_body_count + 1));
            elif_body[elif_body_count++] = stmt;
        }
        // Add to AST
        size_t idx = if_node->if_stmt.elif_count++;
        if_node->if_stmt.elif_conditions = realloc(if_node->if_stmt.elif_conditions, sizeof(ASTNode*) * if_node->if_stmt.elif_count);
        if_node->if_stmt.elif_bodies = realloc(if_node->if_stmt.elif_bodies, sizeof(ASTNode**) * if_node->if_stmt.elif_count);
        if_node->if_stmt.elif_body_counts = realloc(if_node->if_stmt.elif_body_counts, sizeof(size_t) * if_node->if_stmt.elif_count);
        if_node->if_stmt.elif_conditions[idx] = cond_node;
        if_node->if_stmt.elif_bodies[idx] = elif_body;
        if_node->if_stmt.elif_body_counts[idx] = elif_body_count;
    }

    Token *t = peek(p);
    if (t && t->type == TOKEN_ELSE) {
        consume(p); // consume 'else'
        if (!match(p, TOKEN_SYMBOL, "{")) {
            fprintf(stderr, "Expected '{' to start else body\n");
            ast_free(if_node);
            return NULL;
        }
        ASTNode **else_body = NULL;
        size_t else_body_count = 0;
        while (1) {
            Token *et = peek(p);
            if (!et) {
                fprintf(stderr, "Unexpected EOF in else body\n");
                ast_free(if_node);
                return NULL;
            }
            if (et->type == TOKEN_SYMBOL && et->length == 1 && strncmp(et->lexeme, "}", 1) == 0) {
                consume(p);
                break;
            }
            ASTNode *stmt = parse_statement(p);
            if (!stmt) {
                fprintf(stderr, "Failed to parse statement in else body\n");
                ast_free(if_node);
                return NULL;
            }
            else_body = realloc(else_body, sizeof(ASTNode*) * (else_body_count + 1));
            else_body[else_body_count++] = stmt;
        }
        if_node->if_stmt.else_body = else_body;
        if_node->if_stmt.else_count = else_body_count;
    }

    return if_node;
}

static ASTNode *parse_for_statement(Parser *p) {
    if (!match(p, TOKEN_FOR, "for")) {
        parser_error("Expected 'for' keyword", peek(p));
        return NULL;
    }
    if (!match(p, TOKEN_SYMBOL, "(")) {
        parser_error("Expected ( after 'for'", peek(p));
        return NULL;
    }
    ASTNode *init_node = parse_variable_declaration(p);
    Token *init_token = peek(p);
    if (!init_node) {
        parser_error("Expected condition string in for statement", init_token);
        return NULL;
    }
    if (!match(p, TOKEN_SYMBOL, ";")) {
        parser_error("Expected semicolon after initializer", peek(p));
        return NULL;
    }
    ASTNode *cond_node = parse_expression(p);
    if (!cond_node) {
        parser_error("Expected condition after initializer", peek(p));
        return NULL;
    }
    if (!match(p, TOKEN_SYMBOL, ";")) {
        parser_error("Expected semicolon after condition", peek(p));
        return NULL;
    }
    ASTNode *post_node = parse_statement(p);

    if (!post_node) {
        parser_error("Expected post after condition", peek(p));
        return NULL;
    }
     if (!match(p, TOKEN_SYMBOL, ")")) {
        parser_error("Expected ) after 'for'", peek(p));
        return 0;
    }
    if (!match(p, TOKEN_SYMBOL, "{")) {
        parser_error("Expected '{' to start while body", peek(p));
        return NULL;
    }

    ASTNode *for_node = ast_new_for(init_node, cond_node, post_node);

    // Parse statements inside while body
    while (1) {
        Token *t = peek(p);
        if (!t) {
            parser_error("Unexpected EOF in while body", t);
            ast_free(for_node);
            return NULL;
        }
        if (t->type == TOKEN_SYMBOL && t->length == 1 && strncmp(t->lexeme, "}", 1) == 0) {
            consume(p);
            break;
        }
        ASTNode *stmt = parse_statement(p);
        if (!stmt) {
            parser_error("Failed to parse statement in while body", t);
            ast_free(for_node);
            return NULL;
        }
        ast_add_statement(for_node, stmt);
    }
    return for_node;
}

static ASTNode *parse_while_statement(Parser *p) {
    if (!match(p, TOKEN_WHILE, "while")) {
        parser_error("Expected 'while' keyword", peek(p));
        return NULL;
    }
    if (!match(p, TOKEN_SYMBOL, "(")) {
        parser_error("Expected '(' after 'while'", peek(p));
        return NULL;
    }
    ASTNode *cond_node = parse_expression(p);
    if (!cond_node) {
        parser_error("Expeced condition expression in while statement", peek(p));
        return NULL;
    }
    if (!match(p, TOKEN_SYMBOL, ")")) {
        parser_error("Expected ')' after while condition", peek(p));
        return NULL;
    }
    if (!match(p, TOKEN_SYMBOL, "{")) {
        parser_error("Expected '{' to start while body", peek(p));
        return NULL;
    }

    ASTNode *while_node = ast_new_while(cond_node);

    // Parse statements inside while body
    while (1) {
        Token *t = peek(p);
        if (!t) {
            parser_error("Unexpected EOF in while body", t);
            ast_free(while_node);
            return NULL;
        }
        if (t->type == TOKEN_SYMBOL && t->length == 1 && strncmp(t->lexeme, "}", 1) == 0) {
            consume(p);
            break;
        }
        ASTNode *stmt = parse_statement(p);
        if (!stmt) {
            parser_error("Failed to parse statement in while body", t);
            ast_free(while_node);
            return NULL;
        }
        ast_add_statement(while_node, stmt);
    }

    return while_node;
}

static ASTNode *parse_variable_declaration(Parser *p) {
    Token *type_token = peek(p);
    if (!type_token) {
        parser_error("Expected a type keyword", NULL);
        exit(1);
    }

    VarDeclType var_type;
    switch (type_token->type) {
        case TOKEN_KEYWORD_INT:    var_type = VAR_TYPE_INT;    break;
        case TOKEN_KEYWORD_STR:    var_type = VAR_TYPE_STR;    break;
        case TOKEN_KEYWORD_DOUBLE: var_type = VAR_TYPE_DOUBLE; break;
        case TOKEN_KEYWORD_LIST:   var_type = VAR_TYPE_LIST;   break;
        case TOKEN_KEYWORD_ARRAY:  var_type = VAR_TYPE_ARRAY;  break;
        case TOKEN_IDENTIFIER:     var_type = VAR_TYPE_IDENT;  break;
        default:
            parser_error("Unknown variable type", type_token);
            exit(1);
    }

    consume(p); // Consume the type token

    Token *name_token = peek(p);
    if (!name_token || name_token->type != TOKEN_IDENTIFIER) {
        parser_error("Expected identifier after type", name_token);
        exit(1);
    }

    const char *name = name_token->lexeme;
    size_t name_len = name_token->length;

    consume(p); // Consume the identifier

    if (!match(p, TOKEN_SYMBOL, "=")) {
        parser_error("Expected '=' after variable name", peek(p));
        exit(1);
    }

    Token *value_token = peek(p);
    if (!value_token) {
        parser_error("Expected a value after '='", value_token);
        exit(1);
    }

    ASTNode *value_node = NULL;

    switch (value_token->type) {
        case TOKEN_STRING:
            if (var_type != VAR_TYPE_STR) return NULL;
            value_node = xmalloc(sizeof(ASTNode));
            value_node->type = AST_LITERAL_STRING;
            value_node->literal_string.value = strndup(value_token->lexeme, value_token->length);
            break;

        case TOKEN_NUMBER:
            if ((var_type != VAR_TYPE_INT) && (var_type != VAR_TYPE_DOUBLE)) return NULL;
            value_node = xmalloc(sizeof(ASTNode));
            value_node->type = AST_LITERAL_NUMBER;
            value_node->literal_number.value = strndup(value_token->lexeme, value_token->length);
            break;

        case TOKEN_IDENTIFIER:
            if (var_type != VAR_TYPE_IDENT) return NULL;
            value_node = xmalloc(sizeof(ASTNode));
            value_node->type = AST_IDENTIFIER;
            value_node->literal_string.value = strndup(value_token->lexeme, value_token->length);
            break;

        default:
            parser_error("Unsupported value in variable declaration", value_token);
            exit(1);
    }

    consume(p); // Consume the value token

    return ast_new_var_decl(name, name_len, var_type, value_node);
}

static const TokenType operator_tokens[] = {
    TOKEN_OPERATOR_EQ,    // ==
    TOKEN_OPERATOR_NEQ,   // !=
    TOKEN_OPERATOR_LT,    // <
    TOKEN_OPERATOR_LTE,   // <=
    TOKEN_OPERATOR_GT,    // >
    TOKEN_OPERATOR_GTE,   // >=

    TOKEN_PLUS,           // +
    TOKEN_MINUS,          // -
    TOKEN_MULT,           // *
    TOKEN_POW,            // **
    TOKEN_DIV,            // /
    TOKEN_MOD,            // %

    TOKEN_BIT_AND,        // &
    TOKEN_BIT_OR,         // |
    TOKEN_BIT_XOR,        // ^
    TOKEN_BIT_NOR,        // ~(a|b) if you keep it
    TOKEN_BIT_NOT,        // ~

    TOKEN_SHIFT_LEFT,     // <<
    TOKEN_SHIFT_RIGHT,    // >>

    TOKEN_AND_ASSIGN,     // &=
    TOKEN_OR_ASSIGN,      // |=
    TOKEN_XOR_ASSIGN,     // ^=
    TOKEN_SHL_ASSIGN,     // <<=
    TOKEN_SHR_ASSIGN,     // >>=

    TOKEN_LOGICAL_AND,    // &&
    TOKEN_LOGICAL_OR,     // ||

    TOKEN_INC,            // ++
    TOKEN_DEC             // --
};

static int is_operator_token(TokenType type) {
    size_t count = sizeof(operator_tokens) / sizeof(operator_tokens[0]);
    for (size_t i = 0; i < count; i++) {
        if (operator_tokens[i] == type) {
            return 1;
        }
    }
    return 0;
}

static ASTNode *parse_primary(Parser *p) {
    Token *t = peek(p);
    if (!t) return NULL;

    if (t->type == TOKEN_NUMBER) {
        ASTNode *node = ast_new_literal_number(strndup(t->lexeme, t->length));
        consume(p);
        return node;
    }
    if (t->type == TOKEN_STRING) {
        ASTNode *node = ast_new_literal_string(strndup(t->lexeme, t->length));
        consume(p);
        return node;
    }
    if (t->type == TOKEN_IDENTIFIER) {
        ASTNode *node = ast_new_identifier(t->lexeme, t->length);
        consume(p);
        return node;
    }
    // Parenthesized expression
    if (t->type == TOKEN_SYMBOL && t->length == 1 && strncmp(t->lexeme, "(", 1) == 0) {
        consume(p);
        ASTNode *expr = parse_expression(p);
        if (!match(p, TOKEN_SYMBOL, ")")) {
            parser_error("Expected ')' after expression", peek(p));
            ast_free(expr);
            return NULL;
        }
        return expr;
    }
    parser_error("Unexpected token in expression", t);
    return NULL;
}

static ASTNode *parse_expression(Parser *p) {
    // Parse left operand
    ASTNode *left = parse_primary(p);
    if (!left) return NULL;

    // Check for binary operator
    Token *t = peek(p);
    while (t && is_operator_token(t->type)) {
        // Save operator
        char *op = strndup(t->lexeme, t->length);
        consume(p);

        // For increment/decrement, treat as unary if no right operand
        if (t->type == TOKEN_INC || t->type == TOKEN_DEC) {
            ASTNode *node = ast_new_un_exp(op, left);
            free(op);
            left = node;
            t = peek(p);
            continue;
        }

        // Parse right operand
        ASTNode *right = parse_primary(p);
        if (!right) {
            parser_error("Expected right operand after operator", peek(p));
            free(op);
            ast_free(left);
            return NULL;
        }
        ASTNode *node = ast_new_bin_exp(op, left, right);
        free(op);
        left = node;
        t = peek(p);
    }
    return left;
}