/*
	* SA.c - [Enter description]
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
#include "SA.h"
#include "ast.h"
#include "memutils.h"
#include "SA_to_json.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/
extern int debug;
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

/* --- Functions ---*/
int analyze_semantics(Node *ast) {
	if (!ast) {
		fprintf(stderr, "[!] Semantic Analysis: NULL AST provided\n");
        return 1;
	}

	SemanticContext ctx;
    ctx.global_scope = create_scope(NULL, SCOPE_GLOBAL);
    ctx.current_scope = ctx.global_scope;
    ctx.current_function = NULL;
    ctx.current_return_type = NULL;
    ctx.error_count = 0;
    ctx.warning_count = 0;


    pass1(ast, &ctx);
    //pass2(ast, &ctx);
    //pass3(ast, &ctx);

	emit_symbol_table(&ctx);

    return ctx.error_count > 0 ? 1 : 0;
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
    Symbol *existing = lookup_symbol_current(scope, sym->name);
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
    for (Scope *s = scope; s; s->parent) {
        for (size_t i = 0; i < s->symbol_count; i++) {
            if (strcmp(scope->symbols[i]->name, name) == 0) {
                return scope->symbols[i];
            }
        }
    }
    return NULL;
}

void free_symbol(Symbol *sym) {
    if (!sym) return;
    xfree(sym);
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

                for (size_t i = 0; node->if_stmt.if_body[i]; i++) {
                    pass1(node->if_stmt.if_body[i], ctx);
                }

                for (size_t i = 0; i < node->if_stmt.elif_count; i++) {
                    push_scope(ctx, SCOPE_BLOCK);
                    for (size_t j = 0; node->if_stmt.elif_bodies[i][j]; j++) {
                        pass1(node->if_stmt.elif_bodies[i][j], ctx);
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
                    for (size_t i = 0; node->while_stmt.body[i]; i++) {
                        pass1(node->while_stmt.body[i], ctx);
                    }
                } else if (node->type == NODE_DO_WHILE_STMT) {
                    for (size_t i = 0; node->do_while_stmt.body[i]; i++) {
                        pass1(node->do_while_stmt.body[i], ctx);
                    }
                } else { /* for */
                    if (node->for_stmt.init)
                        pass1(node->for_stmt.init, ctx);
        
                    for (size_t i = 0; node->for_stmt.body[i]; i++) {
                        pass1(node->for_stmt.body[i], ctx);
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

            default:
                break;
        }
    }

void collect_typedef_decl(Node *node, SemanticContext *ctx) {
    Symbol *sym = create_symbol(SYM_TYPEDEF, node->typedef_node.name);
    sym->typedef_sym.actual_type = node->typedef_node.type;
    sym->decl_node = node;

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

    if (!add_symbol(ctx->current_scope, sym)) {
        semantic_error(ctx,
                "redefinition of variable \"%s\"",
                node->var_decl.name
        );
        free_symbol(sym);
    }
}

/*--- Pass 2: Resolve types ---*/
/*--- Pass 3: Check semantics ---*/