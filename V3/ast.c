
#include "ast.h"
#include "memutils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

extern const char* name;

/*
 * ASTNode Constructors
 * --------------------
 * Functions for creating AST nodes for each language construct.
 * All fields are initialized to safe defaults.
*/
ASTNode *ast_new_program() {
    ASTNode *node = xmalloc(sizeof(ASTNode));
    node->type = AST_PROGRAM;
    node->program.statements = NULL;
    node->program.count = 0;
    return node;
}

void add_astnode_to_array(ASTNode ***array, size_t *count, ASTNode *node) {
    *array = xrealloc(*array, sizeof(ASTNode*) * (*count + 1));
    (*array)[*count] = node;
    (*count)++;
}

ASTNode *ast_new_function(const char *name) {
    ASTNode *node = xmalloc(sizeof(ASTNode));
    node->type = AST_FUNCTION;
    node->function.name = strdup(name);
    node->function.body = NULL;
    node->function.count = 0;
    return node;
}

ASTNode *ast_new_var_decl(const char *name, size_t name_len, VarDeclType type, ASTNode *value) {
    ASTNode *node = xmalloc(sizeof(ASTNode));
    node->type = AST_VAR_DECL;

    node->var_decl.name = strndup(name, name_len); // Duplicate the variable name
    node->var_decl.name_len = name_len;
    node->var_decl.var_type = type;
    node->var_decl.value = value;

    return node;
}

ASTNode *ast_new_print(ASTNode *value) {
    ASTNode *node = xmalloc(sizeof(ASTNode));
    node->type = AST_PRINT;
    node->print.value = value;
    return node;
}

ASTNode *ast_new_literal_string(const char *value) {
    ASTNode *node = xmalloc(sizeof(ASTNode));
    node->type = AST_LITERAL_STRING;
    node->literal_string.value = strdup(value);
    return node;
}

ASTNode *ast_new_identifier(const char *name, size_t name_len) {
    ASTNode *node = xmalloc(sizeof(ASTNode));
    node->type = AST_IDENTIFIER;
    node->identifier.name = strndup(name, name_len);
    node->identifier.name_len = name_len;
    return node;
}

ASTNode *ast_new_literal_number(const char *value) {
    ASTNode *node = xmalloc(sizeof(ASTNode));
    node->type = AST_LITERAL_NUMBER;
    node->literal_number.value = strdup(value);
    return node;
}

void ast_add_statement(ASTNode *parent, ASTNode *child) {
    if (!parent || !child) return;

    switch (parent->type) {
        case AST_PROGRAM:
            add_astnode_to_array(&parent->program.statements, &parent->program.count, child);
            break;
        case AST_FUNCTION:
            add_astnode_to_array(&parent->function.body, &parent->function.count, child);
            break;
        case AST_IF:
            add_astnode_to_array(&parent->if_stmt.body, &parent->if_stmt.count, child);
            break;
        case AST_WHILE:
            add_astnode_to_array(&parent->while_stmt.body, &parent->while_stmt.count, child);
            break;
        case AST_FOR:
            add_astnode_to_array(&parent->for_stmt.body, &parent->for_stmt.count, child);
            break;
        default:
            fprintf(stderr, "Cannot add statement to node of type %d\n", parent->type);
            break;
    }
}

ASTNode *ast_new_if(ASTNode *condition) {
    ASTNode *node = xmalloc(sizeof(ASTNode));
    node->type = AST_IF;
    node->if_stmt.condition = condition;
    node->if_stmt.body = NULL;
    node->if_stmt.count = 0;
    node->if_stmt.elif_conditions = NULL;
    node->if_stmt.elif_bodies = NULL;
    node->if_stmt.elif_body_counts = NULL;
    node->if_stmt.elif_count = 0;
    node->if_stmt.else_body = NULL;
    node->if_stmt.else_count = 0;
    return node;
}

ASTNode *ast_new_while(ASTNode *condition) {
    ASTNode *node = xmalloc(sizeof(ASTNode));
    node->type = AST_WHILE;
    node->while_stmt.condition = condition;
    node->while_stmt.body = NULL;
    node->while_stmt.count = 0;
    return node;
}

ASTNode *ast_new_for(ASTNode *init, ASTNode *condition, ASTNode *post) {
    ASTNode *node = xmalloc(sizeof(ASTNode));
    node->type = AST_FOR;
    node->for_stmt.init = init;
    node->for_stmt.condition = condition;
    node->for_stmt.post = post;
    node->for_stmt.body = NULL;
    node->for_stmt.count = 0;
    return node;
}

ASTNode *ast_new_bin_exp(const char *op, ASTNode *left, ASTNode *right) {
    ASTNode *node = xmalloc(sizeof(ASTNode));
    node->type = AST_BINARY_EXPR;
    node->binary_expr.op = strdup(op);
    node->binary_expr.left = left;
    node->binary_expr.right = right;
    return node;
}
ASTNode *ast_new_un_exp(const char *op, ASTNode *operand) {
    ASTNode *node = xmalloc(sizeof(ASTNode));
    node->type = AST_UNARY_EXPR;
    node->unary_expr.op = strdup(op);
    node->unary_expr.operand = operand;
    return node;
}

#define MAX_AST_DEPTH 128

static void print_indent(int depth, const bool *has_more_siblings) {
    for (int i = 0; i < depth; i++) {
        printf("%s", has_more_siblings[i] ? "│   " : "    ");
    }
}

static void print_prefix(const char *prefix) {
    printf("%s", prefix);
}

void ast_print_internal(const ASTNode *node, const char *prefix, bool is_last) {
    if (!node) return;

    // Print current node connector and label
    printf("%s%s", prefix, is_last ? "└── " : "├── ");

    // Prepare next prefix for children
    char child_prefix[512];
    
    snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix, is_last ? "    " : "│   ");

    switch (node->type) {
        case AST_PROGRAM:
            printf("\x1b[35m[PROGRAM] %s\x1b[0m\n", name);
            for (size_t i = 0; i < node->program.count; i++) {
                ast_print_internal(node->program.statements[i], child_prefix, i == node->program.count - 1);
            }
            break;

        case AST_FUNCTION:
            printf("\x1b[32m%s()\x1b[0m\n", node->function.name);
            for (size_t i = 0; i < node->function.count; i++) {
                ast_print_internal(node->function.body[i], child_prefix, i == node->function.count - 1);
            }
            break;

        case AST_PRINT:
            printf("\x1b[32mprint()\x1b[0m\n");
            ast_print_internal(node->print.value, child_prefix, true);
            break;
        case AST_IDENTIFIER:
            printf("%.*s\n", (int)node->identifier.name_len, node->identifier.name);
            break;


        case AST_LITERAL_STRING:
            printf("\"%s\"\n", node->literal_string.value);
            break;

        case AST_LITERAL_NUMBER:
            printf("%s\n", node->literal_number.value);
            break;

        case AST_VAR_DECL: {
            const char *type_str = node->var_decl.var_type == VAR_TYPE_INT ? "int" :
                                   node->var_decl.var_type == VAR_TYPE_DOUBLE ? "double" :
                                   node->var_decl.var_type == VAR_TYPE_STR ? "str" :
                                   node->var_decl.var_type == VAR_TYPE_LIST ? "list" :
                                   node->var_decl.var_type == VAR_TYPE_ARRAY ? "array" : "unknown";
            printf("%s %.*s\n", type_str, (int)node->var_decl.name_len, node->var_decl.name);
            if (node->var_decl.value) {
                ast_print_internal(node->var_decl.value, child_prefix, true);
            }
            break;
        }

        case AST_IF:
            printf("\x1b[35mif\x1b[0m\n");

            // Print [COND]
            printf("%s├── [COND]\n", child_prefix);
            {
                char cond_prefix[512];
                snprintf(cond_prefix, sizeof(cond_prefix), "%s│   ", child_prefix);
                ast_print_internal(node->if_stmt.condition, cond_prefix, true);
            }

            // Print [BODY]
            bool has_elif = node->if_stmt.elif_count > 0;
            bool has_else = node->if_stmt.else_count > 0;
            bool body_is_last = !has_elif && !has_else;

            printf("%s%s── [BODY]\n", child_prefix, body_is_last ? "└" : "├");

            char body_prefix[512];
            snprintf(body_prefix, sizeof(body_prefix), "%s%s   ", child_prefix, body_is_last ? "    " : "│");

            for (size_t i = 0; i < node->if_stmt.count; i++) {
                ast_print_internal(node->if_stmt.body[i], body_prefix, i == node->if_stmt.count - 1);
            }

            // Print elif blocks
            for (size_t i = 0; i < node->if_stmt.elif_count; i++) {
                bool is_last_elif = (i == node->if_stmt.elif_count - 1) && node->if_stmt.else_count == 0;
                printf("%s%s── \x1b[35melif\x1b[0m\n", child_prefix, is_last_elif ? "└" : "├");

                char elif_prefix[512];
                snprintf(elif_prefix, sizeof(elif_prefix), "%s%s   ", child_prefix, is_last_elif ? "    " : "│  ");

                // Elif [COND]
                printf("%s├── [COND]\n", elif_prefix);
                {
                    char cond_prefix[512];
                    snprintf(cond_prefix, sizeof(cond_prefix), "%s│   ", elif_prefix);
                    ast_print_internal(node->if_stmt.elif_conditions[i], cond_prefix, true);
                }

                // Elif [BODY]
                printf("%s└── [BODY]\n", elif_prefix);
                {
                    char body_prefix[512];
                    snprintf(body_prefix, sizeof(body_prefix), "%s    ", elif_prefix);
                    for (size_t j = 0; j < node->if_stmt.elif_body_counts[i]; j++) {
                        ast_print_internal(node->if_stmt.elif_bodies[i][j], body_prefix, j == node->if_stmt.elif_body_counts[i] - 1);
                    }
                }
            }

            // Print else block if any
            if (node->if_stmt.else_body) {
                printf("%s└── \x1b[35melse\x1b[0m\n", child_prefix);
                char else_prefix[512];
                snprintf(else_prefix, sizeof(else_prefix), "%s    ", child_prefix);
                printf("%s└── [BODY]\n", else_prefix);
                char body_prefix[512];
                snprintf(body_prefix, sizeof(body_prefix), "%s    ", else_prefix);
                for (size_t i = 0; i < node->if_stmt.else_count; i++) {
                    ast_print_internal(node->if_stmt.else_body[i], body_prefix, i == node->if_stmt.else_count - 1);
                }
            }
            break;

        case AST_WHILE:
            printf("\x1b[36mwhile\x1b[0m\n");

            // [COND]
            printf("%s├── \x1b[35m[COND]\x1b[0m\n", child_prefix);
            {
                char cond_prefix[512];
                snprintf(cond_prefix, sizeof(cond_prefix), "%s│   ", child_prefix);
                ast_print_internal(node->while_stmt.condition, cond_prefix, true);
            }

            // [BODY]
            printf("%s└── \x1b[35m[BODY]\x1b[0m\n", child_prefix);
            {
                char body_prefix[512];
                snprintf(body_prefix, sizeof(body_prefix), "%s    ", child_prefix);
                for (size_t i = 0; i < node->while_stmt.count; i++) {
                    ast_print_internal(node->while_stmt.body[i], body_prefix, i == node->while_stmt.count - 1);
                }
            }
            break;

        case AST_FOR:
            printf("\x1b[36mfor\x1b[0m\n");

            // [INIT]
            printf("%s├── [INIT]\n", child_prefix);
            {
                char init_prefix[512];
                snprintf(init_prefix, sizeof(init_prefix), "%s│   ", child_prefix);
                ast_print_internal(node->for_stmt.init, init_prefix, true);
            }

            // [COND]
            printf("%s├── [COND]\n", child_prefix);
            {
                char cond_prefix[512];
                snprintf(cond_prefix, sizeof(cond_prefix), "%s│   ", child_prefix);
                ast_print_internal(node->for_stmt.condition, cond_prefix, true);
            }

            // [POST]
            printf("%s├── [POST]\n", child_prefix);
            {
                char post_prefix[512];
                snprintf(post_prefix, sizeof(post_prefix), "%s│   ", child_prefix);
                ast_print_internal(node->for_stmt.post, post_prefix, true);
            }

            // [BODY]
            printf("%s└── [BODY]\n", child_prefix);
            {
                char body_prefix[512];
                snprintf(body_prefix, sizeof(body_prefix), "%s    ", child_prefix);
                for (size_t i = 0; i < node->for_stmt.count; i++) {
                    ast_print_internal(node->for_stmt.body[i], body_prefix, i == node->for_stmt.count - 1);
                }
            }
            break;
        case AST_BINARY_EXPR:
            printf("[BINOP %s]\n", node->binary_expr.op);
            ast_print_internal(node->binary_expr.left, child_prefix, false);
            ast_print_internal(node->binary_expr.right, child_prefix, true);
            break;
        case AST_UNARY_EXPR:
            printf("[UNOP %s]\n", node->unary_expr.op);
            ast_print_internal(node->unary_expr.operand, child_prefix, true);
            break;
        default:
            printf("UnknownNodeType\n");
            break;
    }
}

void ast_print(const ASTNode *node) {
    printf("\n[AST]\n");
    ast_print_internal(node, "", true);
}

void ast_free(ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_PROGRAM:
            for (size_t i = 0; i < node->program.count; i++) {
                ast_free(node->program.statements[i]);
            }
            free(node->program.statements);
            break;

        case AST_FUNCTION:
            for (size_t i = 0; i < node->function.count; i++) {
                ast_free(node->function.body[i]);
            }
            free((char*)node->function.name);
            free(node->function.body);
            break;

        case AST_PRINT:
            ast_free(node->print.value);
            break;
        
        case AST_VAR_DECL:
            free((char *)node->var_decl.name);
            ast_free(node->var_decl.value);
            break;

        case AST_LITERAL_STRING:
            free((char*)node->literal_string.value);
            break;

        case AST_LITERAL_NUMBER:
            free((char*)node->literal_number.value);
            break;
        
        case AST_IF:
            for (size_t i = 0; i < node->if_stmt.count; i++)
                ast_free(node->if_stmt.body[i]);
            free(node->if_stmt.body);
            for (size_t i = 0; i < node->if_stmt.elif_count; i++) {
                ast_free(node->if_stmt.elif_conditions[i]);
                for (size_t j = 0; j < node->if_stmt.elif_body_counts[i]; j++)
                    ast_free(node->if_stmt.elif_bodies[i][j]);
                free(node->if_stmt.elif_bodies[i]);
            }
            free(node->if_stmt.elif_conditions);
            free(node->if_stmt.elif_bodies);
            free(node->if_stmt.elif_body_counts);
            if (node->if_stmt.else_body) {
                for (size_t i = 0; i < node->if_stmt.else_count; i++)
                    ast_free(node->if_stmt.else_body[i]);
                free(node->if_stmt.else_body);
            }
            break;
        case AST_WHILE:
            for (size_t i = 0; i < node->while_stmt.count; i++)
                ast_free(node->while_stmt.body[i]);
            free(node->while_stmt.body);
            break;

        case AST_FOR:
            ast_free(node->for_stmt.init);
            ast_free(node->for_stmt.condition);
            ast_free(node->for_stmt.post);
            for (size_t i = 0; i < node->for_stmt.count; i++)
                ast_free(node->for_stmt.body[i]);
            free(node->for_stmt.body);
            break;


        case AST_BINARY_EXPR:
            ast_free(node->binary_expr.left);
            ast_free(node->binary_expr.right);
            free((char*)node->binary_expr.op);
            break;
        case AST_UNARY_EXPR:
            ast_free(node->unary_expr.operand);
            free((char*)node->unary_expr.op);
            break;
        case AST_IDENTIFIER:
            free((char*)node->identifier.name);
            break;
        
        default:
            fprintf(stderr, "Warning: freeing unknown AST node type %d\n", node->type);
            break;
    }

    free(node);
}