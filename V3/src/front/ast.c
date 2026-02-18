/*
	* ast.c - AST nodes and freeing. Owly's treehouse, beautiful but needs regular cleaning.
	* Author:   Amity
	* Date:     Thu Oct 30 01:24:29 2025
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
#include "memutils.h"
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/

/* --- Prototypes ---*/
Node *create_node(NodeType type);
void free_ast(Node *node);
/* --- Main ---*/

/* --- Functions ---*/
Node *create_node(NodeType type) {
	Node *node = xcalloc(1, sizeof(Node));
	node->type = type;
    node->rtype = NULL;
	return node;
}

void free_ast(Node *node) {
    if (!node) return;

    switch (node->type) {
        case NODE_PROGRAM:
            if (node->program.stmts) {
                for (size_t i = 0; node->program.stmts[i]; i++) {
                    free_ast(node->program.stmts[i]);
                }
                xfree(node->program.stmts);
            }
            break;

        case NODE_VAR_DECL:
            free_ast(node->var_decl.type);
            xfree((char *)node->var_decl.name);
            free_expr(node->var_decl.value);
            break;

        case NODE_FUNC_DECL:
            free_ast(node->func_decl.type);
            xfree((char *)node->func_decl.name);

            if (node->func_decl.args) {
                for (size_t i = 0; i < node->func_decl.arg_count; i++) {
                    free_ast(node->func_decl.args[i]);
                }
                xfree(node->func_decl.args);
            }

            if (node->func_decl.body) {
                for (size_t i = 0; i < node->func_decl.body_count; i++) {
                    free_ast(node->func_decl.body[i]);
                }
                xfree(node->func_decl.body);
            }
            break;

        case NODE_RETURN:
            free_expr(node->return_stmt.value);
            break;

        case NODE_EXPR:
            free_expr(node->expr.expr);
            break;

        case NODE_ENUM_DECL:
            if (node->enum_decl.name) {
                xfree((void *)node->enum_decl.name);
            }
            for (size_t i = 0; i < node->enum_decl.member_count; i++) {
                xfree(node->enum_decl.members[i].name);
                free_expr(node->enum_decl.members[i].value); // may be NULL, safe
            }
            xfree(node->enum_decl.members);
            break;

        case NODE_STRUCT_DECL:
            xfree((void *)node->struct_decl.name);
            for (size_t i = 0; i < node->struct_decl.member_count; i++) {
                free_ast(node->struct_decl.members[i]);
            }
            xfree(node->struct_decl.members);
            break;

        case NODE_UNION_DECL:
            xfree((void *)node->union_decl.name);
            for (size_t i = 0; i < node->union_decl.member_count; i++) {
                free_ast(node->union_decl.members[i]);
            }
            xfree(node->union_decl.members);
            break;

        case NODE_WHILE_STMT:
            free_expr(node->while_stmt.cond);
            for (size_t i = 0; node->while_stmt.body[i]; i++) {
                free_ast(node->while_stmt.body[i]);
            }
            xfree(node->while_stmt.body);
            break;

        case NODE_DO_WHILE_STMT:
            for (size_t i = 0; node->do_while_stmt.body[i]; i++) {
                free_ast(node->do_while_stmt.body[i]);
            }
            xfree(node->do_while_stmt.body);
            free_expr(node->do_while_stmt.cond);
            break;

        case NODE_FOR_STMT:
            if (node->for_stmt.init) {
                free_ast(node->for_stmt.init);
            }
            if (node->for_stmt.cond) {
                free_ast(node->for_stmt.cond);
            }
            if (node->for_stmt.inc) {
                free_expr(node->for_stmt.inc);
            }

            for (size_t i = 0; node->for_stmt.body[i]; i++) {
                free_ast(node->for_stmt.body[i]);
            }
            break;

        case NODE_TYPE:
            xfree(node->type_node.spec);
            /* type_node uses a union for base/decl; use is_decl to know
               which field is active to avoid double-free / invalid free. */
            if (node->type_node.is_decl) {
                if (node->type_node.decl) {
                    free_ast(node->type_node.decl);
                }
            } else {
                if (node->type_node.base) {
                    xfree((char *)node->type_node.base);
                }
            }
            break;

        case NODE_IF_STMT:
            free_expr(node->if_stmt.if_cond);
            if (node->if_stmt.if_body) {
                for (size_t i = 0; node->if_stmt.if_body[i]; i++)
                    free_ast(node->if_stmt.if_body[i]);
                xfree(node->if_stmt.if_body);
            }
            if (node->if_stmt.elif_count) {
                for (size_t i = 0; i < node->if_stmt.elif_count; i++) {
                    free_expr(node->if_stmt.elif_conds[i]);
                    if (node->if_stmt.elif_bodies[i]) {
                        for (size_t j = 0; node->if_stmt.elif_bodies[i][j]; j++)
                            free_ast(node->if_stmt.elif_bodies[i][j]);
                        xfree(node->if_stmt.elif_bodies[i]);
                    }
                }
                xfree(node->if_stmt.elif_conds);
                xfree(node->if_stmt.elif_bodies);
            }
            if (node->if_stmt.else_body) {
                for (size_t i = 0; node->if_stmt.else_body[i]; i++)
                    free_ast(node->if_stmt.else_body[i]);
                xfree(node->if_stmt.else_body);
            }
            break;

        case NODE_SWITCH_STMT:
            free_expr(node->switch_stmt.expression);
            if (node->switch_stmt.case_count) {
                for (size_t i = 0; i < node->switch_stmt.case_count; i++) {
                    free_expr(node->switch_stmt.cases[i]);
                    if (node->switch_stmt.case_bodies[i]) {
                        for (size_t j = 0; node->switch_stmt.case_bodies[i][j]; j++)
                            free_ast(node->switch_stmt.case_bodies[i][j]);
                        xfree(node->switch_stmt.case_bodies[i]);
                    }
                }
                xfree(node->switch_stmt.cases);
                xfree(node->switch_stmt.case_bodies);
            }

            for (size_t i = 0; node->switch_stmt.default_body[i]; i++) {
                free_ast(node->switch_stmt.default_body[i]);
            }
            xfree(node->switch_stmt.default_body);
            break;

        case NODE_MISC:
            xfree(node->misc.name);
            break;
        
        case NODE_TYPEDEF:
            xfree(node->typedef_node.name);
            free_ast(node->typedef_node.type);
            break;

        case NODE_ARRAY:
            xfree(node->array.name);
            free_ast(node->array.type);
            free_expr(node->array.value);
            xfree(node->array.dim);
            break;
            
        default:
			printf("[X] How did you get here?\n");
            break;
    }
    xfree(node);
}