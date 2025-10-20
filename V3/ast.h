
#ifndef AST_H
#define AST_H

#include <stddef.h>

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_IDENTIFIER,
    AST_PRINT,
    AST_LITERAL_STRING,
    AST_LITERAL_NUMBER,
    AST_VAR_DECL,
    AST_IF,
    AST_WHILE,
    AST_FOR,

    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
} ASTNodeType;

typedef enum {
    VAR_TYPE_STR,
    VAR_TYPE_INT,
    VAR_TYPE_DOUBLE,
    VAR_TYPE_LIST,
    VAR_TYPE_ARRAY,
    VAR_TYPE_IDENT,
} VarDeclType;

typedef struct ASTNode {
    ASTNodeType type;

    union {
        struct {
            struct ASTNode **statements;
            size_t count;
        } program;

        struct {
            const char *name;
            struct ASTNode **body;
            size_t count;
        } function;

        struct {
            struct ASTNode *value;
        } print;

        struct {
            const char *value;
        } literal_string;

        struct {
            const char *value;
        } literal_number;

        struct {
            const char *name;
            size_t name_len;
            VarDeclType var_type;
            struct ASTNode *value;
        } var_decl; 

        struct {
            /* Condition expression for the if statement */
            struct ASTNode *condition;

            /* Statements inside the if block */
            struct ASTNode **body;
            size_t count;

            /* Elif blocks: conditions and bodies */
            struct ASTNode **elif_conditions;
            struct ASTNode ***elif_bodies;
            size_t *elif_body_counts;
            size_t elif_count;

            /* Else block: statements */
            struct ASTNode **else_body;
            size_t else_count;
        } if_stmt;

        struct {
            struct ASTNode *condition;
            struct ASTNode **body;
            size_t count;
        } while_stmt;

        struct {
            const char *name;
            size_t name_len;
        } identifier;

        struct {
            struct ASTNode *init;
            struct ASTNode *condition;
            struct ASTNode *post;
            struct ASTNode **body;
            size_t count;
        } for_stmt;

        struct {
            char *op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binary_expr;
        
        struct {
            char *op;
            struct ASTNode *operand;
        } unary_expr;
        
    };
} ASTNode;


//constructors
ASTNode *ast_new_program();
ASTNode *ast_new_function(const char *name);
ASTNode *ast_new_print(ASTNode *value);
ASTNode *ast_new_literal_string(const char *value);
ASTNode *ast_new_literal_number(const char *value);
ASTNode *ast_new_var_decl(const char *name, size_t name_len, VarDeclType type, ASTNode *value);
ASTNode *ast_new_if(ASTNode *condition);
ASTNode *ast_new_while(ASTNode *condition);
ASTNode *ast_new_for(ASTNode *init, ASTNode *condition, ASTNode *post);
ASTNode *ast_new_identifier(const char *name, size_t name_len);

ASTNode *ast_new_bin_exp(const char *op, ASTNode *left, ASTNode *right);
ASTNode *ast_new_un_exp(const char *op, ASTNode *operand);


//helpers
void ast_add_statement(ASTNode *parent, ASTNode *child);
void ast_print(const ASTNode *node);
void ast_free(ASTNode *node);

#endif
