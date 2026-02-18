/*
	* SA.c - Semantic analyzer. Owly's inner critic: "This type doesn't match, darling."
	* Author:   Amity
	* Date:     Mon Dec 22 16:57:49 2025
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

#include "front/ast.h"
#include "front/expressions.h"
#include "middle/SA.h"
#include "middle/SA_to_json.h"
#include "memutils.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdalign.h>
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/
extern int debug;
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
void check_array_initialiser(Node *node, SemanticContext *ctx);
int is_narrowing_conversion(ResolvedType *expected, ResolvedType *actual);
MemberInfo find_struct_member(ResolvedType *struct_type, const char *member_name, SemanticContext *ctx);
int is_valid_cast(ResolvedType *source, ResolvedType *target, SemanticContext *ctx);


MemberInfo find_struct_member(ResolvedType *struct_type, const char *member_name, SemanticContext *ctx);
/* --- Functions ---*/
SemanticContext *analyze_semantics(Node *ast) {
    if (!ast) {
        fprintf(stderr, "[!] Semantic Analysis: NULL AST provided\n");
        return NULL;
    }

    SemanticContext *ctx = xmalloc(sizeof(SemanticContext));
    ctx->global_scope = create_scope(NULL, SCOPE_GLOBAL);
    ctx->current_scope = ctx->global_scope;
    ctx->current_function = NULL;
    ctx->current_return_type = NULL;
    ctx->error_count = 0;
    ctx->warning_count = 0;

    printf("[X] Pass 1, collect declarations\n");
    pass1(ast, ctx);
    
    printf("[X] Pass 2, resolve types\n");
    pass2(ast, ctx);

    printf("[X] Pass 3, check semantics\n");
    pass3(ast, ctx);

    emit_symbol_table(ctx);

    return ctx;
}

Scope *create_scope(Scope *parent, ScopeType type) {
    Scope *scope = xmalloc(sizeof(Scope));
    scope->parent = parent;
    scope->children = NULL;
    scope->child_count = 0;
    scope->child_capacity = 0;
    scope->symbols = NULL;
    scope->symbol_count = 0;
    scope->symbol_capacity = 0;
    scope->type = type;
    scope->name = NULL;
    
    if (parent) {
        if (parent->child_count >= parent->child_capacity) {
            size_t new_cap = parent->child_capacity == 0 ? 4 : 
                            parent->child_capacity * 2;
            parent->children = xrealloc(parent->children, 
                                       new_cap * sizeof(Scope *));
            parent->child_capacity = new_cap;
        }
        parent->children[parent->child_count++] = scope;
    }
    
    return scope;
}

Symbol *create_symbol(SymbolKind kind, const char *name) {
    Symbol *sym = xmalloc(sizeof(Symbol));
    sym->kind = kind;
    sym->name = name;
    sym->decl_node = NULL;
    sym->scope = NULL;
    
    memset(&sym->variable, 0, sizeof(sym->variable));
    
    return sym;
}

int add_symbol(Scope *scope, Symbol *sym) {
    Symbol *existing = lookup_symbol_recursive(scope, sym->name);
    if (existing) {
        return 0;
    }
    
    if (scope->symbol_count >= scope->symbol_capacity) {
        size_t new_cap = scope->symbol_capacity == 0 ? 8 : 
                         scope->symbol_capacity * 2;
        scope->symbols = xrealloc(scope->symbols, 
                                  new_cap * sizeof(Symbol *));
        scope->symbol_capacity = new_cap;
    }
    
    sym->scope = scope;
    scope->symbols[scope->symbol_count++] = sym;
    return 1;  // Success!
}

Symbol *lookup_symbol_current(Scope *scope, const char *name) {
    if (!scope) return NULL;
    
    for (size_t i = 0; i < scope->symbol_count; i++) {
        if (strcmp(scope->symbols[i]->name, name) == 0) {
            return scope->symbols[i];
        }
    }
    
    return NULL;
}

Symbol *lookup_symbol_recursive(Scope *scope, const char *name) {
    for (Scope *s = scope; s; s = s->parent) {
        for (size_t i = 0; i < s->symbol_count; i++) {
            if (strcmp(s->symbols[i]->name, name) == 0) {
                return s->symbols[i];
            }
        }
    }
    return NULL;
}

void semantic_error(SemanticContext *ctx, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[!] Error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    ctx->error_count++;
}

void semantic_warning(SemanticContext *ctx, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[!] Warning: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    ctx->warning_count++;
}


void push_scope(SemanticContext *ctx, ScopeType type) {
    Scope *new_scope = create_scope(ctx->current_scope, type);
    ctx->current_scope = new_scope;
}

void pop_scope(SemanticContext *ctx) {
    if (!ctx->current_scope->parent) {
        semantic_error(ctx, "cannot pop global scope!");
        return;
    }
    ctx->current_scope = ctx->current_scope->parent;
}

void enter_scope(SemanticContext *ctx, Scope *scope) {
    if (!scope) {
        return;
    }
    ctx->current_scope = scope;
}
void leave_scope(SemanticContext *ctx) {
    if (ctx->current_scope) {
        ctx->current_scope = ctx->current_scope->parent;
    }
}

size_t align_up(size_t value, size_t align) {
    return (value + align - 1) & ~(align - 1);
}

ResolvedType *copy_resolved_type(ResolvedType *rt) {
    if (!rt) return NULL;

    ResolvedType *copy = xcalloc(1, sizeof(ResolvedType));
    copy->kind = rt->kind;
    copy->name = rt->name;
    copy->decl = rt->decl;
    copy->param_count = rt->param_count;
    copy->is_variadic = rt->is_variadic;
    copy->size = rt->size;
    copy->align = rt->align;
    copy->is_signed = rt->is_signed;
    copy->is_floating = rt->is_floating;
    copy->is_const = rt->is_const;
    copy->is_volatile = rt->is_volatile;
    copy->is_complete = rt->is_complete;

    if (rt->base) {
        copy->base = copy_resolved_type(rt->base);
    }

    if (rt->params && rt->param_count > 0) {
        copy->params = xcalloc(rt->param_count, sizeof(ResolvedType *));
        for (size_t i = 0; i < rt->param_count; i++) {
            copy->params[i] = copy_resolved_type(rt->params[i]);
        }
    }
    if (rt->dimensions && rt->dim_count > 0) {
        copy->dimensions = xcalloc(rt->dim_count, sizeof(size_t));
        memcpy(copy->dimensions, rt->dimensions, rt->dim_count * sizeof(size_t));
    }
    copy->dim_count = rt->dim_count;
    copy->total_elements = rt->total_elements;

    copy->enum_base = rt->enum_base;

    return copy;
}

void free_symbol(Symbol *sym) {
    if (!sym) return;

    if (sym->resolved) {
        free_resolved_type(sym->resolved);
    }
    xfree(sym);
}

void free_resolved_type(ResolvedType *rt) {
    if (!rt) return;

    if (rt->base) {
        free_resolved_type(rt->base);
    }

    if (rt->params) {
        for (size_t i = 0; i < rt->param_count; i++) {
            if (rt->params[i]) {
                free_resolved_type(rt->params[i]);
            }
        }
        xfree(rt->params);
    }

    if (rt->kind == RT_ARRAY && rt->dimensions) {
        xfree(rt->dimensions);
    }

    xfree(rt);
}

void free_scope(Scope *scope) {
    if (!scope) return;

    if (scope->children) {
        for (size_t i = 0; i < scope->child_count; i++) {
            if (scope->children[i]) {
                free_scope(scope->children[i]);
            }
        }
        xfree(scope->children);
    }

    if (scope->symbols) {
        for (size_t i = 0; i < scope->symbol_count; i++) {
            if (scope->symbols[i]) {
                free_symbol(scope->symbols[i]);
            }
        }
        xfree(scope->symbols);
    }

    xfree(scope);
}

void free_semantic_context(SemanticContext *ctx) {
    if (!ctx) return;

    if (ctx->global_scope) {
        free_scope(ctx->global_scope);
        ctx->global_scope = NULL;
    }

    ctx->current_scope = NULL;
    ctx->current_function = NULL;
    ctx->current_return_type = NULL;

    printf("[X] Freed Semantic Analyser successfully\n");
}

/*--- Pass 1: Collect declarations ---*/
void pass1(Node *node, SemanticContext *ctx) {
    if (!node) return;

    switch (node->type) {
        case NODE_PROGRAM: 
            for (size_t i = 0; node->program.stmts[i]; i++) {
                pass1(node->program.stmts[i], ctx);
            }
            break;
        
        case NODE_TYPEDEF:
            collect_typedef_decl(node, ctx);
            break;

        case NODE_ENUM_DECL: 
            collect_enum_decl(node, ctx);
            break;

        case NODE_STRUCT_DECL:
            collect_struct_decl(node, ctx);
            break;

        case NODE_UNION_DECL:
            collect_union_decl(node, ctx);
            break;

        case NODE_FUNC_DECL: 
            collect_function_decl(node, ctx);
            break;
        
        case NODE_VAR_DECL:
            collect_var_decl(node, ctx);
            break;

        case NODE_IF_STMT:
            push_scope(ctx, SCOPE_BLOCK);

            if (node->if_stmt.if_body) {
                for (size_t i = 0; node->if_stmt.if_body[i]; i++) {
                    pass1(node->if_stmt.if_body[i], ctx);
                }
            }

            for (size_t i = 0; i < node->if_stmt.elif_count; i++) {
                push_scope(ctx, SCOPE_BLOCK);
                if (node->if_stmt.elif_bodies[i]) {
                    for (size_t j = 0; node->if_stmt.elif_bodies[i][j]; j++) {
                        pass1(node->if_stmt.elif_bodies[i][j], ctx);
                    }
                }
                pop_scope(ctx);
            }

            if (node->if_stmt.else_body) {
                push_scope(ctx, SCOPE_BLOCK);
                for (size_t i = 0; node->if_stmt.else_body[i]; i++) {
                    pass1(node->if_stmt.else_body[i], ctx);
                }
                pop_scope(ctx);
            }
            pop_scope(ctx);
            break;

        case NODE_WHILE_STMT:
        case NODE_DO_WHILE_STMT:
        case NODE_FOR_STMT:
            push_scope(ctx, SCOPE_BLOCK);

            if (node->type == NODE_WHILE_STMT) {
                if (node->while_stmt.body) {
                    for (size_t i = 0; node->while_stmt.body[i]; i++) {
                        pass1(node->while_stmt.body[i], ctx);
                    }
                }
            } else if (node->type == NODE_DO_WHILE_STMT) {
                if (node->do_while_stmt.body) {
                    for (size_t i = 0; node->do_while_stmt.body[i]; i++) {
                        pass1(node->do_while_stmt.body[i], ctx);
                    }
                }
            } else { /* for */
                if (node->for_stmt.init)
                    pass1(node->for_stmt.init, ctx);
    
                if (node->for_stmt.body) {
                    for (size_t i = 0; node->for_stmt.body[i]; i++) {
                        pass1(node->for_stmt.body[i], ctx);
                    }
                }
            }
            pop_scope(ctx);
            break;

        case NODE_SWITCH_STMT:
            push_scope(ctx, SCOPE_BLOCK);

            for (size_t i = 0; i < node->switch_stmt.case_count; i++) {
                push_scope(ctx, SCOPE_BLOCK);
                for (size_t j = 0; node->switch_stmt.case_bodies[i][j]; j++) {
                    pass1(node->switch_stmt.case_bodies[i][j], ctx);
                }
                pop_scope(ctx);
            }

            if (node->switch_stmt.default_body) {
                push_scope(ctx, SCOPE_BLOCK);
                for (size_t i = 0; node->switch_stmt.default_body[i]; i++) {
                    pass1(node->switch_stmt.default_body[i], ctx);
                }
                pop_scope(ctx);
            }
            pop_scope(ctx);
            break;

        case NODE_ARRAY:
            collect_array_decl(node, ctx);
            break;
        default:
            break;
    }
}

void collect_typedef_decl(Node *node, SemanticContext *ctx) {
    Symbol *sym = create_symbol(SYM_TYPEDEF, node->typedef_node.name);
    sym->typedef_sym.actual_type = node->typedef_node.type;
    sym->decl_node = node;
    sym->scope = ctx->current_scope;

    if (!add_symbol(ctx->current_scope, sym)) {
        semantic_error(ctx,
            "redefinition of typedef \"%s\"",
            node->typedef_node.name
        );
        free_symbol(sym);
        return; 
    }
}

void collect_enum_decl(Node *node, SemanticContext *ctx) {
    Symbol *sym = create_symbol(SYM_ENUM, node->enum_decl.name);
    sym->decl_node = node;
    sym->scope = ctx->current_scope;

    if (!add_symbol(ctx->current_scope, sym)) {
        semantic_error(ctx,
            "redefinition of enum \"%s\"",
            node->enum_decl.name
        );
        free_symbol(sym);
        return;
    }

    push_scope(ctx, SCOPE_ENUM);
    ctx->current_scope->name = node->enum_decl.name;

    for (size_t i = 0; i < node->enum_decl.member_count; i++) {
        EnumMember *member = &node->enum_decl.members[i];
    
        if (!member->name) {
            semantic_error(ctx, "enum member has no name in enum \"%s\"", node->enum_decl.name);
            continue;
        }
    
        Symbol *mem_sym = create_symbol(SYM_ENUM_MEMBER, member->name);
        mem_sym->enum_sym.members = member;
        mem_sym->decl_node = node;
        mem_sym->scope = ctx->current_scope;
    
        if (!add_symbol(ctx->current_scope, mem_sym) ||
            !add_symbol(ctx->current_scope->parent, mem_sym))
        {
            semantic_error(ctx,
                "redefinition of enum member \"%s\"",
                member->name
            );
        }
    }    
    pop_scope(ctx);
}

void collect_struct_decl(Node *node, SemanticContext *ctx) {
    Symbol *sym = create_symbol(SYM_STRUCT, node->struct_decl.name);
    sym->decl_node = node;
    sym->scope = ctx->current_scope;

    if (!add_symbol(ctx->current_scope, sym)) {
        semantic_error(ctx,
            "redefinition of struct \"%s\"",
            node->struct_decl.name
        );
        free_symbol(sym);
    }

    push_scope(ctx, SCOPE_STRUCT);
    ctx->current_scope->name = node->struct_decl.name;

    for (size_t i = 0; i < node->struct_decl.member_count; i++) {
        Node *member = node->struct_decl.members[i];

        if (member->type != NODE_VAR_DECL) {
            semantic_error(ctx,
                "Invalid declaration inside struct \"%s\"",
                node->struct_decl.name
            );
            continue;
        }
        collect_var_decl(member, ctx);
    }
    pop_scope(ctx);
}

void collect_union_decl(Node *node, SemanticContext *ctx) {
    Symbol *sym = create_symbol(SYM_UNION, node->union_decl.name);
    sym->decl_node = node;
    sym->scope = ctx->current_scope;
    
    if (!add_symbol(ctx->current_scope, sym)) {
        semantic_error(ctx,
            "redefinition of union \"%s\"",
            node->union_decl.name
        );
        free_symbol(sym);
        return;
    }

    push_scope(ctx, SCOPE_UNION);
    ctx->current_scope->name = node->union_decl.name;

    for (size_t i = 0; i < node->union_decl.member_count; i++) {
        Node *member = node->union_decl.members[i];

        if (member->type != NODE_VAR_DECL) {
            semantic_error(ctx,
                "Invalid declaration inside union \"%s\"",
                node->union_decl.name
            );
            continue;
        }
        collect_var_decl(member, ctx);
    }
    pop_scope(ctx);
}

void collect_function_decl(Node *node, SemanticContext *ctx) {
    Symbol *sym = create_symbol(SYM_FUNCTION, node->func_decl.name);
    sym->decl_node = node;
    sym->function.params = node->func_decl.args;
    sym->function.param_count = node->func_decl.arg_count;
    sym->function.return_type = node->func_decl.type;
    sym->function.is_defined = !node->func_decl.is_prototype;
    sym->scope = ctx->current_scope;

    /* Add function symbol first (allows recursion) */
    if (!add_symbol(ctx->current_scope, sym)) {
        semantic_error(ctx,
            "redefinition of function \"%s\"",
            node->func_decl.name
        );
        free_symbol(sym);
        return;
    }

    /* Enter function scope */
    push_scope(ctx, SCOPE_FUNCTION);
    ctx->current_scope->name = node->func_decl.name;
    ctx->current_function = node;
    ctx->current_return_type = node->func_decl.type;

    /* Parameters are variable declarations */
    for (size_t i = 0; i < node->func_decl.arg_count; i++) {
        pass1(node->func_decl.args[i], ctx);
    }

    /* Function body */
    for (size_t i = 0; i < node->func_decl.body_count; i++) {
        pass1(node->func_decl.body[i], ctx);
    }

    pop_scope(ctx);

    ctx->current_function = NULL;
    ctx->current_return_type = NULL;
}


void collect_var_decl(Node *node, SemanticContext *ctx) {
    Symbol *sym = create_symbol(SYM_VARIABLE, node->var_decl.name);
    sym->decl_node = node;
    sym->variable.type = node->var_decl.type;
    sym->variable.is_initialized = (node->var_decl.value != NULL);
    sym->scope = ctx->current_scope;

    if (!add_symbol(ctx->current_scope, sym)) {
        semantic_error(ctx,
                "redefinition of variable \"%s\"",
                node->var_decl.name
        );
        free_symbol(sym);
    }
}

void collect_array_decl(Node *node, SemanticContext *ctx) {
    Symbol *sym = create_symbol(SYM_VARIABLE, node->array.name);
    sym->decl_node = node;

    sym->variable.type = node->array.type;
    sym->variable.is_initialized = (node->array.value != NULL);

    if (!add_symbol(ctx->current_scope, sym)) {
        semantic_error(ctx, "Redeclaration of array '%s'", node->array.name);
        free_symbol(sym);
    }
}

/*--- Pass 2: Resolve types ---*/
void pass2(Node *node, SemanticContext *ctx) {
    if (!node) return;

    switch (node->type) {
        case NODE_PROGRAM:
            for (size_t i = 0; node->program.stmts[i]; i++) {
                pass2(node->program.stmts[i], ctx);
            }
            break;

        case NODE_VAR_DECL:
            node->rtype = resolve_var_type(node, ctx);
            break;

        case NODE_FUNC_DECL: {
            Symbol *func_sym = lookup_symbol_recursive(ctx->current_scope, node->func_decl.name);
            
            if (func_sym && func_sym->scope->child_count > 0) {
                /* Find the function scope created in pass1 */
                Scope *func_scope = NULL;
                for (size_t i = 0; i < func_sym->scope->child_count; i++) {
                    if (func_sym->scope->children[i]->type == SCOPE_FUNCTION &&
                        func_sym->scope->children[i]->name &&
                        strcmp(func_sym->scope->children[i]->name, node->func_decl.name) == 0) {
                        func_scope = func_sym->scope->children[i];
                        break;
                    }
                }
                
                if (func_scope) {
                    enter_scope(ctx, func_scope);
                    
                    /* Resolve parameter types */
                    for (size_t i = 0; i < node->func_decl.arg_count; i++) {
                        pass2(node->func_decl.args[i], ctx);
                    }
                    
                    /* Resolve function return type */
                    resolve_func_type(node, ctx);
                    node->rtype = lookup_symbol_recursive(ctx->current_scope, node->func_decl.name)->resolved;
                    
                    /* Recursively resolve body statements */
                    for (size_t i = 0; i < node->func_decl.body_count; i++) {
                        pass2(node->func_decl.body[i], ctx);
                    }
                    
                    leave_scope(ctx);
                }
            }
            break;
        }

        case NODE_TYPEDEF:
            node->rtype = resolve_typedef(node, ctx);
            break;

        case NODE_ENUM_DECL:
            node->rtype = resolve_enum(node, ctx);
            break;

        case NODE_STRUCT_DECL:
            node->rtype = resolve_struct(node, ctx);
            break;

        case NODE_UNION_DECL:
            node->rtype = resolve_union(node, ctx);
            break;

        case NODE_IF_STMT: {
            size_t child_idx = 0;
            
            /* If body */
            if (child_idx < ctx->current_scope->child_count) {
                Scope *if_scope = ctx->current_scope->children[child_idx++];
                enter_scope(ctx, if_scope);
                
                if (node->if_stmt.if_body) {
                    for (size_t i = 0; node->if_stmt.if_body[i]; i++) {
                        pass2(node->if_stmt.if_body[i], ctx);
                    }
                }
                
                leave_scope(ctx);
            }
            
            /* Elif bodies */
            for (size_t i = 0; i < node->if_stmt.elif_count; i++) {
                if (child_idx < ctx->current_scope->child_count) {
                    Scope *elif_scope = ctx->current_scope->children[child_idx++];
                    enter_scope(ctx, elif_scope);
                    
                    if (node->if_stmt.elif_bodies[i]) {
                        for (size_t j = 0; node->if_stmt.elif_bodies[i][j]; j++) {
                            pass2(node->if_stmt.elif_bodies[i][j], ctx);
                        }
                    }
                    
                    leave_scope(ctx);
                }
            }
            
            /* Else body */
            if (node->if_stmt.else_body && child_idx < ctx->current_scope->child_count) {
                Scope *else_scope = ctx->current_scope->children[child_idx++];
                enter_scope(ctx, else_scope);
                
                for (size_t i = 0; node->if_stmt.else_body[i]; i++) {
                    pass2(node->if_stmt.else_body[i], ctx);
                }
                
                leave_scope(ctx);
            }
            break;
        }

        case NODE_WHILE_STMT:
        case NODE_DO_WHILE_STMT:
        case NODE_FOR_STMT: {
            if (ctx->current_scope->child_count > 0) {
                Scope *loop_scope = ctx->current_scope->children[0];
                enter_scope(ctx, loop_scope);
                
                if (node->type == NODE_WHILE_STMT) {
                    if (node->while_stmt.body) {
                        for (size_t i = 0; node->while_stmt.body[i]; i++) {
                            pass2(node->while_stmt.body[i], ctx);
                        }
                    }
                } else if (node->type == NODE_DO_WHILE_STMT) {
                    if (node->do_while_stmt.body) {
                        for (size_t i = 0; node->do_while_stmt.body[i]; i++) {
                            pass2(node->do_while_stmt.body[i], ctx);
                        }
                    }
                } else { /* FOR */
                    if (node->for_stmt.init)
                        pass2(node->for_stmt.init, ctx);
                    
                    if (node->for_stmt.body) {
                        for (size_t i = 0; node->for_stmt.body[i]; i++) {
                            pass2(node->for_stmt.body[i], ctx);
                        }
                    }
                }
                
                leave_scope(ctx);
            }
            break;
        }

        case NODE_SWITCH_STMT: {
            size_t child_idx = 0;
            
            if (ctx->current_scope->child_count > 0) {
                Scope *switch_scope = ctx->current_scope->children[child_idx++];
                enter_scope(ctx, switch_scope);
                
                /* Process case bodies */
                for (size_t i = 0; i < node->switch_stmt.case_count; i++) {
                    if (child_idx - 1 < ctx->current_scope->child_count) {
                        Scope *case_scope = ctx->current_scope->children[child_idx++ - 1];
                        enter_scope(ctx, case_scope);
                        
                        if (node->switch_stmt.case_bodies[i]) {
                            for (size_t j = 0; node->switch_stmt.case_bodies[i][j]; j++) {
                                pass2(node->switch_stmt.case_bodies[i][j], ctx);
                            }
                        }
                        
                        leave_scope(ctx);
                    }
                }
                
                /* Process default body */
                if (node->switch_stmt.default_body && child_idx - 1 < ctx->current_scope->child_count) {
                    Scope *default_scope = ctx->current_scope->children[child_idx - 1];
                    enter_scope(ctx, default_scope);
                    
                    for (size_t i = 0; node->switch_stmt.default_body[i]; i++) {
                        pass2(node->switch_stmt.default_body[i], ctx);
                    }
                    
                    leave_scope(ctx);
                }
                
                leave_scope(ctx);
            }
            break;
        }

        case NODE_RETURN:
        case NODE_EXPR:
        case NODE_MISC:
            break;

        case NODE_ARRAY: {
            node->rtype = resolve_array(node, ctx);
            break;
        }
        default:
            break;
    }
}

ResolvedType *resolve_type(Node *type_node, SemanticContext *ctx) {
    if (!type_node || type_node->type != NODE_TYPE) {
        return NULL;
    }

    if (!type_node->type_node.spec) {
        return NULL;
    }

    ResolvedType *base = NULL;

    /* Step 1: Resolve base type */
    if (type_node->type_node.is_decl) {
        /* struct/enum/union declaration */
        Node *decl = type_node->type_node.decl;

        switch (decl->type) {
            case NODE_STRUCT_DECL:
                base = resolve_struct(decl, ctx);
                break;
            case NODE_ENUM_DECL:
                base = resolve_enum(decl, ctx);
                break;
            case NODE_UNION_DECL:
                base = resolve_union(decl, ctx);
                break;
            default:
                semantic_error(ctx, "invalid type declaration");
                return NULL;
        }
    } else {
        /* Builtin or typedef */
        const char *name = type_node->type_node.base;
        Symbol *sym = lookup_symbol_recursive(ctx->current_scope, name);
    
        if (sym && sym->kind == SYM_TYPEDEF) {
            base = copy_resolved_type(sym->resolved);
        } else if (sym && sym->kind == SYM_STRUCT) {
            base = copy_resolved_type(sym->resolved);
        } else if (sym && sym->kind == SYM_UNION) {
            base = copy_resolved_type(sym->resolved);
        } else if (sym && sym->kind == SYM_ENUM) {
            base = copy_resolved_type(sym->resolved);
        } else {
            /* Builtin type */
            base = resolve_builtin(name, type_node->type_node.spec, ctx);
        }
    }

    if (!base) {
        return NULL;
    }

    /* Step 2: Apply pointer depth */
    for (int i = 0; i < type_node->type_node.spec->pointer_depth; i++) {
        ResolvedType *ptr = xmalloc(sizeof(ResolvedType));
        *ptr = (ResolvedType){
            .kind = RT_POINTER,
            .base = base,
            .is_complete = 1,
            .size = sizeof(void *),
            .align = _Alignof(void *),
        };
        base = ptr;
    }

    /* Step 3: Apply qualifiers */
    base->is_const |= type_node->type_node.spec->is_const;
    base->is_volatile |= type_node->type_node.spec->is_volatile;

    return base;
}

ResolvedType *resolve_var_type(Node *node, SemanticContext *ctx) {
    if (!node || node->type != NODE_VAR_DECL) {
        return NULL;
    }

    /* Look up the variable symbol in current scope */
    Symbol *sym = lookup_symbol_recursive(ctx->current_scope, node->var_decl.name);

    if (!sym) {
        semantic_error(ctx,
            "variable \"%s\" not found in current scope",
            node->var_decl.name
        );
        return NULL;
    }

    /* Resolve the type node */
    ResolvedType *rt = resolve_type(node->var_decl.type, ctx);

    if (rt) {
        sym->resolved = rt;
    }

    return rt;
}

ResolvedType *resolve_func_type(Node *node, SemanticContext *ctx) {
    if (!node || node->type != NODE_FUNC_DECL) {
        return NULL;
    }

    /* Look up the function symbol in current scope */
    Symbol *sym = lookup_symbol_recursive(ctx->current_scope,
                                          node->func_decl.name);

    if (!sym) {
        semantic_error(ctx,
            "function \"%s\" not found in current scope",
            node->func_decl.name
        );
        return NULL;
    }

    /* Resolve the return type */
    ResolvedType *return_type = resolve_type(node->func_decl.type,
                                             ctx);

    if (!return_type) {
        semantic_error(ctx,
            "failed to resolve return type for function \"%s\"",
            node->func_decl.name
        );
        return NULL;
    }

    /* Create function ResolvedType wrapping the return type */
    ResolvedType *func_rt = xmalloc(sizeof(ResolvedType));
    *func_rt = (ResolvedType){
        .kind = RT_FUNCTION,
        .name = node->func_decl.name,
        .base = return_type,
        .params = NULL,
        .param_count = 0,
        .is_variadic = 0,
        .is_complete = 1
    };

    /* Resolve parameter types */
    if (node->func_decl.arg_count > 0) {
        func_rt->params = xmalloc(node->func_decl.arg_count *
                                  sizeof(ResolvedType *));
        func_rt->param_count = node->func_decl.arg_count;

        for (size_t i = 0; i < node->func_decl.arg_count; i++) {
            Node *param_node = node->func_decl.args[i];

            if (!param_node || param_node->type != NODE_VAR_DECL) {
                semantic_error(ctx, "invalid parameter %zu in function \"%s\"", i, node->func_decl.name);
                xfree(func_rt->params);
                xfree(func_rt);
                return NULL;
            }
            ResolvedType *param_type = resolve_type(param_node->var_decl.type, ctx);
            if (!param_type) {
                semantic_error(ctx, "failed to resolve type for parameter \"%s\" in function \"%s\"", param_node->var_decl.name, node->func_decl.name);
                xfree(func_rt->params);
                xfree(func_rt);
                return NULL;
            }
            func_rt->params[i] = param_type;
        }
    }

    sym->resolved = func_rt;
    return func_rt;
}

ResolvedType *resolve_typedef(Node *node, SemanticContext *ctx) {
    if (!node || node->type != NODE_TYPEDEF) {
        return NULL;
    }

    Symbol *sym = lookup_symbol_recursive(ctx->current_scope, node->typedef_node.name);

    if (!sym) {
        semantic_error(ctx, "typedef \"%s\" not found in current scope", node->typedef_node.name);
        return NULL;
    }

    Node *var_decl = node->typedef_node.type;
    if (!var_decl || var_decl->type != NODE_TYPE) {
        semantic_error(ctx, "invalid typedef structure");
        return NULL;
    }

    Node *type_node = var_decl->var_decl.type;
    ResolvedType *rt = resolve_type(type_node, ctx);

    if (!rt) {
        return NULL;
    }

    sym->resolved = rt;
    return rt;
}

ResolvedType *resolve_enum(Node *node, SemanticContext *ctx) {
    if (!node || node->type != NODE_ENUM_DECL) {
        return NULL;
    }
    Symbol *sym = lookup_symbol_recursive(ctx->current_scope, node->enum_decl.name);
    if (!sym) {
        return NULL;
    }
    if (sym->resolved) {
        return NULL;
    }

    ResolvedType *rt = xcalloc(1, sizeof(ResolvedType));
    rt->kind = RT_ENUM;
    rt->name = node->enum_decl.name;
    rt->is_complete = 1;

    Scope *enum_scope = NULL;
    for (size_t i = 0; i < sym->scope->child_count; i++) {
        Scope *child = sym->scope->children[i];
        if (child->type == SCOPE_ENUM && child->name && strcmp(child->name, sym->name) == 0) {
            enum_scope = child;
            break;
        }
    }

    if (!enum_scope) {
        semantic_error(ctx, "internal error: enum scope not found");
        return rt;
    }

    rt->size = 4;
    rt->align = 4;

    sym->resolved = rt;
    return rt;
}

ResolvedType *resolve_struct(Node *node, SemanticContext *ctx) {
    if (!node || node->type != NODE_STRUCT_DECL)
        return NULL;

    Symbol *sym = lookup_symbol_recursive(ctx->current_scope, node->struct_decl.name);
    if (!sym) {
        return NULL;
    }

    if (sym->resolved) {
        return sym->resolved;
    }

    ResolvedType *rt = xcalloc(1, sizeof(ResolvedType));
    rt->kind = RT_STRUCT;
    rt->name = node->struct_decl.name;
    rt->is_complete = 1;
    rt->decl = node;

    /* Enter struct scope */
    Scope *struct_scope = NULL;
    for (size_t i = 0; i < sym->scope->child_count; i++) {
        Scope *child = sym->scope->children[i];
        if (child->type == SCOPE_STRUCT && child->name && strcmp(child->name, sym->name) == 0) {
            struct_scope = child;
            break;
        }
    }

    if (!struct_scope) {
        semantic_error(ctx, "internal error: struct scope not found");
        return rt;
    }

    enter_scope(ctx, struct_scope);

    size_t offset = 0;
    size_t max_align = 1;

    for (size_t i = 0; i < node->struct_decl.member_count; i++) {
        Node *member = node->struct_decl.members[i];
        if (member->type != NODE_VAR_DECL)
            continue;

        Symbol *member_sym =
            lookup_symbol_recursive(ctx->current_scope, member->var_decl.name);
        if (!member_sym)
            continue;

        ResolvedType *mt = resolve_type(member->var_decl.type, ctx);
        if (!mt)
            continue;

        member_sym->resolved = mt;

        /* Align */
        if (mt->align > max_align)
            max_align = mt->align;
        offset = (offset + mt->align - 1) & ~(mt->align - 1);

        offset += mt->size;
    }

    leave_scope(ctx);

    rt->size = (offset + max_align - 1) & ~(max_align - 1);
    rt->align = max_align;

    sym->resolved = rt;
    return rt;
}

ResolvedType *resolve_union(Node *node, SemanticContext *ctx) {
    if (!node || node->type != NODE_UNION_DECL) {
        return NULL;
    }

    ResolvedType *rt = xcalloc(1, sizeof(ResolvedType));
    rt->kind = RT_UNION;
    rt->name = node->union_decl.name;
    rt->decl = node;
    rt->is_complete = 1;
    
    /* Union size is the max of all members */
    size_t max_size = 0;
    size_t max_align = 1;
    
    /* Find and enter the union's child scope */
    Scope *union_scope = NULL;
    Symbol *sym = lookup_symbol_recursive(ctx->current_scope, node->union_decl.name);
    if (sym && sym->scope) {
        /* Find the child scope matching this union */
        for (size_t i = 0; i < sym->scope->child_count; i++) {
            if (sym->scope->children[i]->type == SCOPE_UNION &&
                sym->scope->children[i]->name &&
                strcmp(sym->scope->children[i]->name, node->union_decl.name) == 0) {
                union_scope = sym->scope->children[i];
                break;
            }
        }
    }
    
    if (union_scope) {
        enter_scope(ctx, union_scope);
    }
    
    for (size_t i = 0; i < node->union_decl.member_count; i++) {
        Node *member = node->union_decl.members[i];
        
        if (member->type == NODE_VAR_DECL && member->var_decl.type) {
            /* Resolve the member's type */
            ResolvedType *member_type = resolve_type(
                member->var_decl.type, ctx
            );
            
            if (member_type) {
                if (member_type->size > max_size) {
                    max_size = member_type->size;
                }
                if (member_type->align > max_align) {
                    max_align = member_type->align;
                }
                
                /* Also attach resolved type to the member symbol */
                Symbol *member_sym = lookup_symbol_recursive(ctx->current_scope, member->var_decl.name);
                if (member_sym) {
                    member_sym->resolved = member_type;
                }
            }
        }
    }
    
    if (union_scope) {
        leave_scope(ctx);
    }
    
    rt->size = max_size;
    rt->align = max_align > 0 ? max_align : 1;
    
    /* Attach to symbol if it exists */
    if (sym) {
        sym->resolved = rt;
    }
    
    return rt;
}

ResolvedType *resolve_array(Node *node, SemanticContext *ctx) {
    ResolvedType *elem_type = resolve_type(node->array.type, ctx);

    if (!elem_type) {
        semantic_error(ctx, "Invalid array element type");
        return NULL;
    }

    ResolvedType *arr_type = xcalloc(1, sizeof(ResolvedType));
    arr_type->kind = RT_ARRAY;
    arr_type->name = node->array.name;
    arr_type->base = elem_type;

    arr_type->dim_count = node->array.dim_count;
    arr_type->dimensions = xcalloc(arr_type->dim_count, sizeof(size_t));
    memcpy(arr_type->dimensions, node->array.dim, arr_type->dim_count * sizeof(size_t));

    arr_type->total_elements = 1;
    for (size_t i = 0; i < arr_type->dim_count; i++) {
        arr_type->total_elements *= arr_type->dimensions[i];
    }

    arr_type->size = elem_type->size * arr_type->total_elements;
    arr_type->align = elem_type->align;
    arr_type->is_complete = 1;

    Symbol *sym = lookup_symbol_current(ctx->current_scope, node->array.name);
    if (sym) {
        sym->resolved = arr_type;
    }
    return arr_type;
}

ResolvedType *resolve_builtin(const char *name, TypeSpec *spec, SemanticContext *ctx) {
    if (!name) {
        return NULL;
    }

    ResolvedType *rt = xmalloc(sizeof(ResolvedType));
    
    /* Default initialization */
    *rt = (ResolvedType){
        .kind = RT_BUILTIN,
        .name = name,
        .is_signed = 1,
        .is_floating = 0,
        .is_complete = 1
    };

    /* Determine signedness from spec */
    int is_unsigned = (spec && spec->sign == UNSIGNED_T);
    rt->is_signed = !is_unsigned;

    /* Classify builtin type based on base name and length modifier */
    if (strcmp(name, "int") == 0) {
        /* Handle length modifiers: short, long, long long */
        if (spec && spec->length == SHORT_T) {
            rt->name = "short";
            rt->size = sizeof(short);
            rt->align = _Alignof(short);
        } else if (spec && spec->length == LONG_T) {
            rt->name = "long";
            rt->size = sizeof(long);
            rt->align = _Alignof(long);
        } else if (spec && spec->length == LONGLONG_T) {
            rt->name = "long long";
            rt->size = sizeof(long long);
            rt->align = _Alignof(long long);
        } else {
            /* Plain int */
            rt->size = sizeof(int);
            rt->align = _Alignof(int);
        }
    } else if (strcmp(name, "void") == 0) {
        rt->size = 0;
        rt->align = 0;
        rt->is_complete = 0;
    } else if (strcmp(name, "char") == 0) {
        rt->size = sizeof(char);
        rt->align = _Alignof(char);
    } else if (strcmp(name, "short") == 0) {
        rt->size = sizeof(short);
        rt->align = _Alignof(short);
    } else if (strcmp(name, "long") == 0) {
        /* "long" without "int" is treated as "long int" */
        if (spec && spec->length == LONGLONG_T) {
            rt->name = "long long";
            rt->size = sizeof(long long);
            rt->align = _Alignof(long long);
        } else {
            rt->size = sizeof(long);
            rt->align = _Alignof(long);
        }
    } else if (strcmp(name, "float") == 0) {
        rt->size = sizeof(float);
        rt->align = _Alignof(float);
        rt->is_floating = 1;
    } else if (strcmp(name, "double") == 0) {
        rt->size = sizeof(double);
        rt->align = _Alignof(double);
        rt->is_floating = 1;
    } else {
        semantic_error(ctx, "unknown builtin type \"%s\"", name);
        rt->kind = RT_INVALID;
    }

    return rt;
}

/*--- Pass 3: Check semantics ---*/
void pass3(Node *node, SemanticContext *ctx) {
    if (!node) return;

    switch (node->type) {
        case NODE_PROGRAM:
            for (size_t i = 0; node->program.stmts[i]; i++) {
                pass3(node->program.stmts[i], ctx);
            }
            break;

        case NODE_VAR_DECL:
            /* Check variable declaration */
            if (node->var_decl.value) {
                ExprTypeInfo init_type = infer_expr_type(node->var_decl.value, ctx);
                Symbol *sym = lookup_symbol_recursive(ctx->current_scope, node->var_decl.name);
                
                if (sym && sym->resolved && init_type.type) {
                    if (!types_compatible(sym->resolved, init_type.type)) {
                        semantic_error(ctx, 
                            "type mismatch in initialization of variable '%s'",
                            node->var_decl.name);
                    } else if (is_narrowing_conversion(sym->resolved, init_type.type)) {
                        semantic_warning(ctx,
                            "implicit narrowing conversion in initialization of variable '%s' (from %s to %s)",
                            node->var_decl.name,
                            init_type.type->name ? init_type.type->name : "unknown",
                            sym->resolved->name ? sym->resolved->name : "unknown");
                    }
                }
            }
            break;

        case NODE_FUNC_DECL: {
            Symbol *func_sym = lookup_symbol_recursive(ctx->current_scope, node->func_decl.name);
            
            if (func_sym && func_sym->scope->child_count > 0) {
                /* Find the function scope */
                Scope *func_scope = NULL;
                for (size_t i = 0; i < func_sym->scope->child_count; i++) {
                    if (func_sym->scope->children[i]->type == SCOPE_FUNCTION &&
                        func_sym->scope->children[i]->name &&
                        strcmp(func_sym->scope->children[i]->name, node->func_decl.name) == 0) {
                        func_scope = func_sym->scope->children[i];
                        break;
                    }
                }
                
                if (func_scope) {
                    enter_scope(ctx, func_scope);
                    
                    Node *old_func = ctx->current_function;
                    Node *old_ret = ctx->current_return_type;
                    ctx->current_function = node;
                    ctx->current_return_type = node->func_decl.type;
                    
                    /* Check function body */
                    for (size_t i = 0; i < node->func_decl.body_count; i++) {
                        pass3(node->func_decl.body[i], ctx);
                    }
                    
                    ctx->current_function = old_func;
                    ctx->current_return_type = old_ret;
                    
                    leave_scope(ctx);
                }
            }
            break;
        }

        case NODE_RETURN:
            /* Check return statement type compatibility */
            if (ctx->current_return_type) {
                ResolvedType *expected = resolve_type(ctx->current_return_type, ctx);
                
                if (node->return_stmt.value) {
                    ExprTypeInfo ret_type = infer_expr_type(node->return_stmt.value, ctx);
                    
                    if (expected && ret_type.type) {
                        if (!types_compatible(expected, ret_type.type)) {
                            semantic_error(ctx, "return type mismatch");
                        }
                    }
                } else {
                    /* Returning void - check if function expects void */
                    if (expected && expected->kind != RT_BUILTIN) {
                        semantic_error(ctx, "non-void function must return a value");
                    }
                }
            }
            break;

        case NODE_EXPR:
            /* Type-check the expression */
            infer_expr_type(node->expr.expr, ctx);
            break;

        case NODE_IF_STMT: {
            /* Check condition is valid */
            if (node->if_stmt.if_cond) {
                infer_expr_type(node->if_stmt.if_cond, ctx);
            }
            
            size_t child_idx = 0;
            
            /* If body */
            if (child_idx < ctx->current_scope->child_count) {
                Scope *if_scope = ctx->current_scope->children[child_idx++];
                enter_scope(ctx, if_scope);
                
                if (node->if_stmt.if_body) {
                    for (size_t i = 0; node->if_stmt.if_body[i]; i++) {
                        pass3(node->if_stmt.if_body[i], ctx);
                    }
                }
                
                leave_scope(ctx);
            }
            
            /* Elif bodies */
            for (size_t i = 0; i < node->if_stmt.elif_count; i++) {
                if (node->if_stmt.elif_conds[i]) {
                    infer_expr_type(node->if_stmt.elif_conds[i], ctx);
                }
                
                if (child_idx < ctx->current_scope->child_count) {
                    Scope *elif_scope = ctx->current_scope->children[child_idx++];
                    enter_scope(ctx, elif_scope);
                    
                    if (node->if_stmt.elif_bodies[i]) {
                        for (size_t j = 0; node->if_stmt.elif_bodies[i][j]; j++) {
                            pass3(node->if_stmt.elif_bodies[i][j], ctx);
                        }
                    }
                    
                    leave_scope(ctx);
                }
            }
            
            /* Else body */
            if (node->if_stmt.else_body && child_idx < ctx->current_scope->child_count) {
                Scope *else_scope = ctx->current_scope->children[child_idx++];
                enter_scope(ctx, else_scope);
                
                for (size_t i = 0; node->if_stmt.else_body[i]; i++) {
                    pass3(node->if_stmt.else_body[i], ctx);
                }
                
                leave_scope(ctx);
            }
            break;
        }

        case NODE_WHILE_STMT:
        case NODE_DO_WHILE_STMT:
        case NODE_FOR_STMT: {
            /* Enter loop scope first (for loops need init/cond variables in scope) */
            if (ctx->current_scope->child_count > 0) {
                Scope *loop_scope = ctx->current_scope->children[0];
                enter_scope(ctx, loop_scope);
                
                if (node->type == NODE_WHILE_STMT) {
                    /* Check condition inside scope */
                    if (node->while_stmt.cond) {
                        infer_expr_type(node->while_stmt.cond, ctx);
                    }
                    
                    if (node->while_stmt.body) {
                        for (size_t i = 0; node->while_stmt.body[i]; i++) {
                            pass3(node->while_stmt.body[i], ctx);
                        }
                    }
                } else if (node->type == NODE_DO_WHILE_STMT) {
                    if (node->do_while_stmt.body) {
                        for (size_t i = 0; node->do_while_stmt.body[i]; i++) {
                            pass3(node->do_while_stmt.body[i], ctx);
                        }
                    }
                    
                    /* Check condition inside scope */
                    if (node->do_while_stmt.cond) {
                        infer_expr_type(node->do_while_stmt.cond, ctx);
                    }
                } else { /* FOR */
                    /* Init variable is declared in this scope */
                    if (node->for_stmt.init)
                        pass3(node->for_stmt.init, ctx);
                    
                    /* Condition uses variables from init */
                    if (node->for_stmt.cond)
                        pass3(node->for_stmt.cond, ctx);
                    
                    /* Increment expression */
                    if (node->for_stmt.inc)
                        infer_expr_type(node->for_stmt.inc, ctx);
                    
                    /* Body */
                    if (node->for_stmt.body) {
                        for (size_t i = 0; node->for_stmt.body[i]; i++) {
                            pass3(node->for_stmt.body[i], ctx);
                        }
                    }
                }
                
                leave_scope(ctx);
            }
            break;
        }

        case NODE_SWITCH_STMT: {
            /* Check switch expression */
            if (node->switch_stmt.expression) {
                infer_expr_type(node->switch_stmt.expression, ctx);
            }
            
            size_t child_idx = 0;
            
            if (ctx->current_scope->child_count > 0) {
                Scope *switch_scope = ctx->current_scope->children[child_idx++];
                enter_scope(ctx, switch_scope);
                
                /* Process case bodies */
                for (size_t i = 0; i < node->switch_stmt.case_count; i++) {
                    if (node->switch_stmt.cases[i]) {
                        infer_expr_type(node->switch_stmt.cases[i], ctx);
                    }
                    
                    if (child_idx - 1 < ctx->current_scope->child_count) {
                        Scope *case_scope = ctx->current_scope->children[child_idx++ - 1];
                        enter_scope(ctx, case_scope);
                        
                        if (node->switch_stmt.case_bodies[i]) {
                            for (size_t j = 0; node->switch_stmt.case_bodies[i][j]; j++) {
                                pass3(node->switch_stmt.case_bodies[i][j], ctx);
                            }
                        }
                        
                        leave_scope(ctx);
                    }
                }
                
                /* Process default body */
                if (node->switch_stmt.default_body && child_idx - 1 < ctx->current_scope->child_count) {
                    Scope *default_scope = ctx->current_scope->children[child_idx - 1];
                    enter_scope(ctx, default_scope);
                    
                    for (size_t i = 0; node->switch_stmt.default_body[i]; i++) {
                        pass3(node->switch_stmt.default_body[i], ctx);
                    }
                    
                    leave_scope(ctx);
                }
                
                leave_scope(ctx);
            }
            break;
        }

        case NODE_ARRAY:
            check_array_initialiser(node, ctx);
            break;

        default:
            /* Other node types don't require checking */
            break;
    }
}

ExprTypeInfo infer_expr_type(Expr *expr, SemanticContext *ctx) {
    ExprTypeInfo info = {0};
    info.type = NULL;
    info.is_lvalue = 0;
    info.is_addressable = 0;
    
    if (!expr) {
        return info;
    }
    
    switch (expr->kind) {
        case EXPR_LITERAL: {
            info.type = xcalloc(1, sizeof(ResolvedType));
            info.type->kind = RT_BUILTIN;
            info.type->is_complete = 1;

            switch (*expr->literal->kind) {
                case LIT_STRING: {
                    ResolvedType *char_type = xcalloc(1, sizeof(ResolvedType));
                    char_type->kind = RT_BUILTIN;
                    char_type->name = "char";
                    char_type->size = sizeof(char);
                    char_type->align = _Alignof(char);
                    char_type->is_const = 1;
                    
                    info.type->kind = RT_POINTER;
                    info.type->base = char_type;
                    info.type->size = sizeof(void*);
                    info.type->align = _Alignof(void*);
                    break;
                }
                case LIT_CHAR: {
                    info.type->name = "char";
                    info.type->size = sizeof(char);
                    info.type->align = _Alignof(char);
                    info.type->is_signed = 1;
                    break;
                }
                case LIT_FLOAT: {
                    size_t len = strlen(expr->literal->raw);
                    if ((expr->literal->raw[len - 1] == 'f' || expr->literal->raw[len - 1] == 'F')) {
                        info.type->name = "float";
                        info.type->size = sizeof(float);
                        info.type->align = _Alignof(float);
                    } else {
                        info.type->name = "double";
                        info.type->size = sizeof(double);
                        info.type->align = _Alignof(double);
                    }
                    info.type->is_floating = 1;
                    break;
                }
                case LIT_INT: {
                    info.type->name = "int";
                    info.type->size = sizeof(int);
                    info.type->align = _Alignof(int);
                    info.type->is_signed = 1;
                    break;
                }
                case LIT_BOOL: {
                    info.type->name = "bool";
                    info.type->size = sizeof(int);
                    info.type->align = _Alignof(int);
                    break;
                }
            }
            info.is_lvalue = 0;
            break;
        }
        
        case EXPR_IDENTIFIER: {
            Symbol *sym = lookup_symbol_recursive(ctx->current_scope, expr->identifier);
            if (!sym) {
                semantic_error(ctx, "undefined identifier '%s'", expr->identifier);
                break;
            }
            
            if (sym->kind == SYM_VARIABLE) {
                info.type = copy_resolved_type(sym->resolved);
                info.is_lvalue = 1;
                info.is_addressable = 1;
            } else if (sym->kind == SYM_ENUM_MEMBER) {
                info.type = xcalloc(1, sizeof(ResolvedType));
                info.type->kind = RT_BUILTIN;
                info.type->name = "int";
                info.type->size = sizeof(int);
                info.type->align = _Alignof(int);
                info.type->is_signed = 1;
                info.type->is_complete = 1;
                info.is_lvalue = 0;
            } else {
                semantic_error(ctx, "'%s' is not a variable", expr->identifier);
            }
            break;
        }
        
        case EXPR_UNARY: {
            ExprTypeInfo operand_info = infer_expr_type(expr->unary.operand, ctx);
            
            if (strcmp(expr->unary.op, "&") == 0) {
                if (!operand_info.is_addressable) {
                    semantic_error(ctx, "cannot take address of non-lvalue");
                }
                info.type = xcalloc(1, sizeof(ResolvedType));
                info.type->kind = RT_POINTER;
                info.type->base = operand_info.type;
                info.type->size = sizeof(void*);
                info.type->align = _Alignof(void*);
                info.type->is_complete = 1;
                info.is_lvalue = 0;
            } else if (strcmp(expr->unary.op, "*") == 0) {
                if (operand_info.type && operand_info.type->kind == RT_POINTER) {
                    info.type = copy_resolved_type(operand_info.type->base);
                    info.is_lvalue = 1;
                    info.is_addressable = 1;
                } else {
                    semantic_error(ctx, "cannot dereference non-pointer type");
                    free_resolved_type(operand_info.type);
                }
            } else if (strcmp(expr->unary.op, "++") == 0 || strcmp(expr->unary.op, "--") == 0) {
                if (!operand_info.is_lvalue) {
                    semantic_error(ctx, "increment/decrement requires lvalue");
                }
                info.type = operand_info.type;
                info.is_lvalue = (expr->unary.order == 1);
            } else if (strcmp(expr->unary.op, "!") == 0 ||
                       strcmp(expr->unary.op, "~") == 0 ||
                       strcmp(expr->unary.op, "+") == 0 ||
                       strcmp(expr->unary.op, "-") == 0) {
                info.type = operand_info.type;
                info.is_lvalue = 0;
            } else {
                semantic_error(ctx, "unknown unary operator '%s'", expr->unary.op);
                free_resolved_type(operand_info.type);
            }
            break;
        }
        
        case EXPR_BINARY: {
            ExprTypeInfo left_info = infer_expr_type(expr->binary.left, ctx);
            ExprTypeInfo right_info = infer_expr_type(expr->binary.right, ctx);

            if (strcmp(expr->binary.op, "+") == 0 || strcmp(expr->binary.op, "-") == 0) {
                if ((left_info.type->kind == RT_POINTER && right_info.type->kind == RT_BUILTIN) || (left_info.type->kind == RT_BUILTIN && right_info.type->kind == RT_POINTER)) {

                    if (left_info.type->kind == RT_POINTER) {
                        info.type = left_info.type;
                    } else {
                        info.type = right_info.type;
                    }
                    info.is_lvalue = 0;
                    break;
                }
            }

            if (strcmp(expr->binary.op, "=") == 0 ||
                strcmp(expr->binary.op, "+=") == 0 ||
                strcmp(expr->binary.op, "-=") == 0 ||
                strcmp(expr->binary.op, "*=") == 0 ||
                strcmp(expr->binary.op, "/=") == 0 ||
                strcmp(expr->binary.op, "%=") == 0 ||
                strcmp(expr->binary.op, "&=") == 0 ||
                strcmp(expr->binary.op, "|=") == 0 ||
                strcmp(expr->binary.op, "^=") == 0 ||
                strcmp(expr->binary.op, "<<=") == 0 ||
                strcmp(expr->binary.op, ">>=") == 0) {
                
                if (!left_info.is_lvalue) {
                    semantic_error(ctx, "left side of assignment must be lvalue");
                }
                
                if (!types_compatible(left_info.type, right_info.type)) {
                    semantic_warning(ctx, "type mismatch in assignment");
                }
                
                info.type = copy_resolved_type(left_info.type);
                info.is_lvalue = 0;
                
                free_resolved_type(right_info.type);
            } else if (strcmp(expr->binary.op, "==") == 0 ||
                     strcmp(expr->binary.op, "!=") == 0 ||
                     strcmp(expr->binary.op, "<") == 0 ||
                     strcmp(expr->binary.op, ">") == 0 ||
                     strcmp(expr->binary.op, "<=") == 0 ||
                     strcmp(expr->binary.op, ">=") == 0 ||
                     strcmp(expr->binary.op, "&&") == 0 ||
                     strcmp(expr->binary.op, "||") == 0) {
                
                info.type = xcalloc(1, sizeof(ResolvedType));
                info.type->kind = RT_BUILTIN;
                info.type->name = "int";
                info.type->size = sizeof(int);
                info.type->align = _Alignof(int);
                info.type->is_signed = 1;
                info.type->is_complete = 1;
                info.is_lvalue = 0;
                
                free_resolved_type(left_info.type);
                free_resolved_type(right_info.type);
            } else {
                info.type = left_info.type;
                info.is_lvalue = 0;
                free_resolved_type(right_info.type);
            }
            break;
        }
        
        case EXPR_GROUPING: {
            info = infer_expr_type(expr->group.expr, ctx);
            break;
        }
        
        case EXPR_CALL: {
            Symbol *func_sym = lookup_symbol_recursive(ctx->current_scope, expr->call.func);
            if (!func_sym) {
                semantic_error(ctx, "undefined function '%s'", expr->call.func);
                break;
            }
            
            if (func_sym->kind != SYM_FUNCTION) {
                semantic_error(ctx, "'%s' is not a function", expr->call.func);
                break;
            }

            if (expr->call.arg_count != func_sym->function.param_count) {
                semantic_error(ctx, "function '%s' expects %zu arguments, got %zu", expr->call.func, func_sym->function.param_count, expr->call.arg_count);
            }
            for (size_t i = 0; i < expr->call.arg_count && i < func_sym->function.param_count; i++) {
                ExprTypeInfo arg_info = infer_expr_type(expr->call.args[i], ctx);
                
                Node *param = func_sym->function.params[i];
                if (param && param->type == NODE_VAR_DECL) {
                    Symbol *param_sym = lookup_symbol_recursive(func_sym->scope->children[0], param->var_decl.name);
                    
                    if (param_sym && param_sym->resolved) {
                        if (!types_compatible(param_sym->resolved, arg_info.type)) {
                            semantic_warning(ctx, "argument %zu type mismatch in call to '%s'", i + 1, expr->call.func);
                        }
                    }
                }
                
                free_resolved_type(arg_info.type);
            }
            
            /* Return type is the function's return type */
            if (func_sym->resolved && func_sym->resolved->base) {
                info.type = copy_resolved_type(func_sym->resolved->base);
            }
            info.is_lvalue = 0;
            break;
        }
        
        case EXPR_MEMBER: {
            ExprTypeInfo obj_info = infer_expr_type(expr->member.object, ctx);
            if (!obj_info.type) {
                semantic_error(ctx, "Cannot access member of unknown type");
                break;
            }

            ResolvedType *struct_type = obj_info.type;

            if (expr->member.is_arrow) {
                if (struct_type->kind != RT_POINTER) {
                    semantic_error(ctx, "Cannot use -> on non-pointer type");
                    break;
                }
                struct_type = struct_type->base;
            }

            MemberInfo member_info = find_struct_member(struct_type, expr->member.member, ctx);
            if (!member_info.found) {
                semantic_error(ctx, "Struct/union has no member named '%s'", expr->member.member);
                break;
            }
            expr->member.offset = member_info.offset;

            info.type = member_info.type;
            info.is_lvalue = 1;
            info.is_addressable = 1;
            break;
        }

        case EXPR_SIZEOF: {
            size_t size = 0;

            if (expr->sizeof_expr.is_type) {
                ResolvedType *rt = resolve_type(expr->sizeof_expr.type, ctx);
                if (rt && rt->is_complete) {
                    size = rt->size;
                } else {
                    semantic_error(ctx, "sizeof applied to incomplete type");
                }
            } else {
                ExprTypeInfo operand_info = infer_expr_type(expr->sizeof_expr.expr, ctx);
                if (operand_info.type && operand_info.type->is_complete) {
                    size = operand_info.type->size;
                } else {
                    semantic_error(ctx, "sizeof applied to incomplete type");
                }
            }

            expr->sizeof_expr.computed_size = size;

            TypeSpec *spec = xcalloc(1, sizeof(TypeSpec));
            spec->storage = STORAGE_NONE;
            spec->sign = UNSIGNED_T;
            spec->length = LONG_T;
            info.type = resolve_builtin("int", spec, ctx);
            info.is_lvalue = 0;
            info.is_addressable = 0;
            break;
        }

        case EXPR_CAST: {
            ResolvedType *target = resolve_type(expr->cast.target_type, ctx);
            if (!target) {
                semantic_error(ctx, "Invalid cast target type");
                break;
            }

            ExprTypeInfo source_info = infer_expr_type(expr->cast.expr, ctx);
            if (!source_info.type) {
                semantic_error(ctx, "Cannot cast expression with unknown type");
                break;
            }

            if (!is_valid_cast(source_info.type, target, ctx)) {
                break;
            }

            info.type = target;
            info.is_lvalue = 0;
            info.is_addressable = 0;
            break;
        }

        case EXPR_SET: {
            if (expr->set.element_count == 0) {
                semantic_error(ctx, "Empty set expression");
                break;
            }

            ExprTypeInfo first_elem = infer_expr_type(expr->set.elements[0], ctx);
            info.type = first_elem.type;
            info.is_lvalue = 0;
            info.is_addressable = 0;

            for (size_t i = 1; i < expr->set.element_count; i++) {
                ExprTypeInfo elem_info = infer_expr_type(expr->set.elements[i], ctx);
                if (!types_compatible(first_elem.type, elem_info.type)) {
                    semantic_warning(ctx, "Set elements have mismatched types");
                }
            }
            break;
        }

        case EXPR_INDEX: {
            ExprTypeInfo arr_info = infer_expr_type(expr->index.array, ctx);
            
            if (!arr_info.type) {
                semantic_error(ctx, "Cannot infer type of array expression");
                break;
            }

            if (arr_info.type->kind != RT_ARRAY) {
                semantic_error(ctx, "Cannot index non-array type, got kind %d", arr_info.type->kind);
                break;
            }
            ExprTypeInfo idx_info = infer_expr_type(expr->index.index, ctx);
            if (idx_info.type && idx_info.type->kind == RT_BUILTIN && idx_info.type->is_floating) {
                semantic_error(ctx, "Array index must be integer, not float");
            }
            
            ResolvedType *result_type;
            
            if (arr_info.type->dim_count > 1) {
                ResolvedType *sub_array = xcalloc(1, sizeof(ResolvedType));
                sub_array->kind = RT_ARRAY;
                sub_array->base = arr_info.type->base;
                sub_array->dim_count = arr_info.type->dim_count - 1;
                sub_array->dimensions = xcalloc(sub_array->dim_count, sizeof(size_t));

                for (size_t i = 0; i < sub_array->dim_count; i++) {
                    sub_array->dimensions[i] = arr_info.type->dimensions[i + 1];
                }
                
                sub_array->total_elements = 1;
                for (size_t i = 0; i < sub_array->dim_count; i++) {
                    sub_array->total_elements *= sub_array->dimensions[i];
                }
                sub_array->size = sub_array->total_elements * sub_array->base->size;
                sub_array->align = sub_array->base->align;
                sub_array->is_complete = 1;
                
                result_type = sub_array;
            } else {
                result_type = arr_info.type->base;
            }
            
            info.type = result_type;
            info.is_lvalue = 1;
            info.is_addressable = 1;
            break;
        }

        default:
            semantic_error(ctx, "unknown expression kind");
            break;
    }

    if (info.type) {
        expr->inferred_type = info.type;
    }
    
    return info;
}

int types_compatible(ResolvedType *expected, ResolvedType *actual) {
    if (!expected || !actual) return 0;
    
    /* Same pointer means identical types */
    if (expected == actual) return 1;
    
    /* Different kinds are incompatible, except for numeric conversions */
    if (expected->kind != actual->kind) {
        /* Allow numeric conversions between builtin types */
        if (expected->kind == RT_BUILTIN && actual->kind == RT_BUILTIN) {
            /* Both are numeric types, allow implicit conversions */
            return 1;
        }
        return 0;
    }
    
    switch (expected->kind) {
        case RT_BUILTIN:
            /* Builtin types must match by name */
            if (!expected->name || !actual->name) return 0;
            
            /* Exact match */
            if (strcmp(expected->name, actual->name) == 0) {
                return 1;
            }
            
            /* Allow implicit numeric conversions */
            /* char, short, int, long, long long, float, double */
            const char *numeric_types[] = {
                "char", "short", "int", "long", "float", "double", NULL
            };
            
            int expected_is_numeric = 0;
            int actual_is_numeric = 0;
            
            for (int i = 0; numeric_types[i]; i++) {
                if (strcmp(expected->name, numeric_types[i]) == 0) {
                    expected_is_numeric = 1;
                }
                if (strcmp(actual->name, numeric_types[i]) == 0) {
                    actual_is_numeric = 1;
                }
            }
            
            /* Both numeric = compatible */
            if (expected_is_numeric && actual_is_numeric) {
                return 1;
            }
            
            return 0;
            
        case RT_POINTER:
            /* Pointers are compatible if their base types are compatible */
            /* Special case: void* is compatible with any pointer */
            if (expected->base && expected->base->kind == RT_BUILTIN &&
                expected->base->name && strcmp(expected->base->name, "void") == 0) {
                return 1;
            }
            if (actual->base && actual->base->kind == RT_BUILTIN &&
                actual->base->name && strcmp(actual->base->name, "void") == 0) {
                return 1;
            }
            
            return types_compatible(expected->base, actual->base);
            
        case RT_STRUCT:
        case RT_UNION:
        case RT_ENUM:
            /* Named types are compatible if they point to same declaration */
            if (expected->decl && actual->decl) {
                return expected->decl == actual->decl;
            }
            
            /* Or if they have the same name */
            if (expected->name && actual->name) {
                return strcmp(expected->name, actual->name) == 0;
            }
            
            return 0;
            
        case RT_FUNCTION:
            /* Functions compatible if return types and params match */
            if (!types_compatible(expected->base, actual->base)) {
                return 0;
            }
            
            if (expected->param_count != actual->param_count) {
                return 0;
            }
            
            for (size_t i = 0; i < expected->param_count; i++) {
                if (!types_compatible(expected->params[i], actual->params[i])) {
                    return 0;
                }
            }
            
            return 1;
            
        case RT_INVALID:
        default:
            return 0;
    }
}

int is_narrowing_conversion(ResolvedType *expected, ResolvedType *actual) {
    if (!expected || !actual) return 0;
    
    /* Only check builtin numeric types */
    if (expected->kind != RT_BUILTIN || actual->kind != RT_BUILTIN) {
        return 0;
    }
    
    /* Not narrowing if types are the same */
    if (expected->name && actual->name && 
        strcmp(expected->name, actual->name) == 0) {
        return 0;
    }
    
    /* Floating point to integer is narrowing */
    if (actual->is_floating && !expected->is_floating) {
        return 1;
    }
    
    /* Larger to smaller type is narrowing */
    if (expected->size < actual->size) {
        return 1;
    }
    
    return 0;
}

void check_array_initialiser(Node *node, SemanticContext *ctx) {
    if (!node->array.value) return;

    Symbol *sym = lookup_symbol_current(ctx->current_scope, node->array.name);
    if (!sym || !sym->resolved) return;
    
    ResolvedType *arr_type = sym->resolved;

    if (node->array.value->kind != EXPR_SET) {
        semantic_error(ctx, "Array initializer must be a set {...}");
        return;
    }

    size_t provided = node->array.value->set.element_count;
    size_t expected = arr_type->dimensions[0];
    
    if (provided > expected) {
        semantic_error(ctx, "Too many initializers for array (got %zu, expected %zu)", provided, expected);
    }
    for (size_t i = 0; i < provided; i++) {
        Expr *elem = node->array.value->set.elements[i];
        ExprTypeInfo elem_info = infer_expr_type(elem, ctx);
        
        if (!types_compatible(arr_type->base, elem_info.type)) {
            semantic_error(ctx, "Array initializer element type mismatch");
        }
    }
}

MemberInfo find_struct_member(ResolvedType *struct_type, const char *member_name, SemanticContext *ctx) {
    MemberInfo info = {0};

    if (!struct_type || (struct_type->kind != RT_STRUCT && struct_type->kind != RT_UNION)) {
        return info;
    }

    if (!struct_type->decl) return info;
    
    Node *decl = struct_type->decl;
    Node **members = NULL;
    size_t member_count = 0;

    if (decl->type == NODE_STRUCT_DECL) {
        members = decl->struct_decl.members;
        member_count = decl->struct_decl.member_count;
    } else if (decl->type == NODE_UNION_DECL) {
        members = decl->union_decl.members;
        member_count = decl->union_decl.member_count;
    } else {
        return info;
    }

    size_t offset = 0;
    for (size_t i = 0; i < member_count; i++) {
        Node *member = members[i];
        if (member->type != NODE_VAR_DECL) continue;
        
        if (strcmp(member->var_decl.name, member_name) == 0) {
            /* Found it! */
            ResolvedType *member_type = resolve_type(member->var_decl.type, ctx);
            info.type = member_type;
            info.offset = offset;
            info.found = 1;
            return info;
        }

        if (struct_type->kind == RT_STRUCT) {
            ResolvedType *member_type = resolve_type(member->var_decl.type, ctx);
            if (member_type) {
                offset = align_up(offset, member_type->align);
                offset += member_type->size;
            }
        }
    }
    return info;
}

int is_valid_cast(ResolvedType *source, ResolvedType *target, SemanticContext *ctx) {
    if (!source || !target) {
        return 0;
    }

    if (types_compatible(source, target)) return 1;

    int source_is_scalar = (source->kind == RT_BUILTIN || source->kind == RT_POINTER || source->kind == RT_ENUM);
    int target_is_scalar = (target->kind == RT_BUILTIN || target->kind == RT_POINTER || target->kind == RT_ENUM);

    if (source_is_scalar && target_is_scalar) {
        return 1;
    }

    if (source->kind == RT_STRUCT || source->kind == RT_UNION) {
        semantic_error(ctx, "Cannot cast struct/union type");
        return 0;
    }

    if (target->kind == RT_STRUCT || target->kind == RT_UNION) {
        semantic_error(ctx, "Cannot cast to struct/union type");
        return 0;
    }

    if (source->kind == RT_FUNCTION || target->kind == RT_FUNCTION) {
        semantic_error(ctx, "Cannot cast function type");
        return 0;
    }

    semantic_error(ctx, "Invalid cast from %s to %s", source->name ? source->name : "unknown", target->name ? target->name : "unknown");
    return 0;
}