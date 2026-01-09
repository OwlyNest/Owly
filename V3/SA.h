/*
	* SA.h - [Enter description]
	* Author:   Amity
	* Date:     Mon Dec 22 16:57:43 2025
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
#ifndef SA_H
#define SA_H
/* --- Includes ---*/
#include "ast.h"
#include "expressions.h"


#include <cjson/cJSON.h>
/* --- Typedefs - Structs - Enums ---*/
typedef enum {
    SYM_VARIABLE,
    SYM_FUNCTION,
    SYM_TYPEDEF,
    SYM_ENUM,
    SYM_STRUCT,
    SYM_UNION,
    SYM_ENUM_MEMBER
} SymbolKind;

typedef struct Symbol {
    SymbolKind kind;
    const char *name;
    Node *decl_node;                    /* Pointer to declaration */
    struct Scope *scope;                /* Scope where defined */
    
    union {
        struct {
            Node *type;
            int is_initialized;
        } variable;
        
        struct {
            Node *return_type;
            Node **params;
            size_t param_count;
            int is_defined;             /* vs just declared */
        } function;
        
        struct {
            Node *actual_type;          /* Points to struct/enum/etc */
        } typedef_sym;
        
        struct {
            EnumMember *members;
            size_t member_count;
        } enum_sym;

        struct {
            Expr *value_expr;
            int value;
        } enum_member;
        
        struct {
            Node **members;
            size_t member_count;
        } struct_sym;
    };
} Symbol;

typedef enum {
    SCOPE_GLOBAL,
    SCOPE_FUNCTION,
    SCOPE_BLOCK,
    SCOPE_STRUCT,
    SCOPE_ENUM,
    SCOPE_UNION,
} ScopeType;

typedef struct Scope {
    struct Scope *parent;               /* NULL for global scope */
    struct Scope **children;            /* Child scopes */
    size_t child_count;
    size_t child_capacity;
    Symbol **symbols;                   /* Dynamic array */
    size_t symbol_count;
    size_t symbol_capacity;
    ScopeType type;
    const char *name;
} Scope;

typedef struct {
    Scope *global_scope;
    Scope *current_scope;
    Node *current_function;
    Node *current_return_type;
    int error_count;
    int warning_count;
    Token *token_list;
    size_t token_count;
} SemanticContext;

/* --- Globals ---*/

/* --- Prototypes ---*/
int analyze_semantics(Node *ast);
Scope *create_scope(Scope *parent, ScopeType type);
Symbol *create_symbol(SymbolKind kind, const char *name);
int add_symbol(Scope *scope, Symbol *sym);
Symbol *lookup_symbol_current(Scope *scope, const char *name);
Symbol *lookup_symbol_recursive(Scope *scope, const char *name);
void free_symbol(Symbol *sym);
void semantic_error(SemanticContext *ctx, const char *fmt, ...);
void semantic_warning(SemanticContext *ctx, const char *fmt, ...);
void push_scope(SemanticContext *ctx, ScopeType type);
void pop_scope(SemanticContext *ctx);
void pass1(Node *node, SemanticContext *ctx);
void collect_typedef_decl(Node *node, SemanticContext *ctx);
void collect_enum_decl(Node *node, SemanticContext *ctx);
void collect_struct_decl(Node *node, SemanticContext *ctx);
void collect_union_decl(Node *node, SemanticContext *ctx);
void collect_function_decl(Node *node, SemanticContext *ctx);
void collect_var_decl(Node *node, SemanticContext *ctx);
#endif