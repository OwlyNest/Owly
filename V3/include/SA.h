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
#include "front/ast.h"
#include "front/expressions.h"

#include <cjson/cJSON.h>
/* --- Typedefs - Structs - Enums ---*/
typedef struct {
    ResolvedType *type;
    size_t offset;
    int found;
} MemberInfo;

typedef enum {
    RT_INVALID,
    RT_BUILTIN,
    RT_POINTER,
    RT_STRUCT,
    RT_UNION,
    RT_ENUM,
    RT_FUNCTION,
    RT_ARRAY,
} ResolvedTypeKind;

typedef struct ResolvedType {
    ResolvedTypeKind kind;

    const char *name;              // "int", "Foo", etc. NOT OWNED
    Node *decl;                    // Points to AST, NOT OWNED

    struct ResolvedType *base;     // For pointers/functions, OWNED
    struct ResolvedType **params;  // For functions, OWNED
    size_t param_count;
    int is_variadic;

    struct ResolvedType *enum_base; // For enums, OWNED

    size_t size;
    size_t align;

    int is_signed;
    int is_floating;
    int is_const;
    int is_volatile;

    int is_complete;

    size_t *dimensions;
    size_t dim_count;
    size_t total_elements;
} ResolvedType;


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

    ResolvedType *resolved;
    
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
            Node *actual_type;       
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

typedef struct {
    ResolvedType *type;
    int is_lvalue;
    int is_addressable;
} ExprTypeInfo;

typedef enum {
    // Type errors
    SE_TYPE_MISMATCH_ASSIGNMENT,
    SE_TYPE_MISMATCH_BINARY_OP,
    SE_TYPE_MISMATCH_FUNCTION_ARG,
    SE_TYPE_MISMATCH_RETURN,
    
    // Undefined references
    SE_UNDEFINED_VARIABLE,
    SE_UNDEFINED_FUNCTION,
    
    // Control flow
    SE_MISSING_RETURN,
    SE_UNREACHABLE_CODE,
    SE_RETURN_IN_VOID,
    SE_RETURN_VALUE_MISMATCH,
    
    // Function issues
    SE_FUNCTION_NOT_DEFINED,
    SE_ARGUMENT_COUNT_MISMATCH,
    
    // Scope issues
    SE_REDECLARATION,
    SE_OUT_OF_SCOPE,
} SemanticErrorType;

typedef struct {
    int in_function;
    int in_loop;
    int must_return;
    int has_return_path;
} CheckContext;
/* --- Globals ---*/

/* --- Prototypes ---*/
SemanticContext *analyze_semantics(Node *ast);
Scope *create_scope(Scope *parent, ScopeType type);
Symbol *create_symbol(SymbolKind kind, const char *name);
int add_symbol(Scope *scope, Symbol *sym);
Symbol *lookup_symbol_current(Scope *scope, const char *name);
Symbol *lookup_symbol_recursive(Scope *scope, const char *name);
void semantic_error(SemanticContext *ctx, const char *fmt, ...);
void semantic_warning(SemanticContext *ctx, const char *fmt, ...);
void push_scope(SemanticContext *ctx, ScopeType type);
void pop_scope(SemanticContext *ctx);
void enter_scope(SemanticContext *ctx, Scope *scope);
void leave_scope(SemanticContext *ctx);
size_t align_up(size_t value, size_t align);
ResolvedType *copy_resolved_type(ResolvedType *rt);


void free_symbol(Symbol *sym);
void free_resolved_type(ResolvedType *rt);
void free_scope(Scope *scope);
void free_semantic_context(SemanticContext *ctx);

void pass1(Node *node, SemanticContext *ctx);
void collect_typedef_decl(Node *node, SemanticContext *ctx);
void collect_enum_decl(Node *node, SemanticContext *ctx);
void collect_struct_decl(Node *node, SemanticContext *ctx);
void collect_union_decl(Node *node, SemanticContext *ctx);
void collect_function_decl(Node *node, SemanticContext *ctx);
void collect_var_decl(Node *node, SemanticContext *ctx);
void collect_array_decl(Node *node, SemanticContext *ctx);

void pass2(Node *node, SemanticContext *ctx);
ResolvedType *resolve_type(Node *type_node, SemanticContext *ctx);
ResolvedType *resolve_var_type(Node *node, SemanticContext *ctx);
ResolvedType *resolve_func_type(Node *node, SemanticContext *ctx);
ResolvedType *resolve_typedef(Node *node, SemanticContext *ctx);
ResolvedType *resolve_enum(Node *node, SemanticContext *ctx);
ResolvedType *resolve_struct(Node *node, SemanticContext *ctx);
ResolvedType *resolve_union(Node *node, SemanticContext *ctx);
ResolvedType *resolve_array(Node *node, SemanticContext *ctx);
ResolvedType *resolve_builtin(const char *name, TypeSpec *spec, SemanticContext *ctx);

void pass3(Node *node, SemanticContext *ctx);
ExprTypeInfo infer_expr_type(Expr *expr, SemanticContext *ctx);
int types_compatible(ResolvedType *expected, ResolvedType *actual);
int is_narrowing_conversion(ResolvedType *expected, ResolvedType *actual);
void check_array_initialiser(Node *node, SemanticContext *ctx);
MemberInfo find_struct_member(ResolvedType *struct_type, const char *member_name, SemanticContext *ctx);
int is_valid_cast(ResolvedType *source, ResolvedType *target, SemanticContext *ctx);
#endif