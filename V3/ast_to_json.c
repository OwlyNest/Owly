/*
	* ast_to_json.c - [Enter description]
	* Author:   Amity
	* Date:     Thu Oct 30 14:51:57 2025
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
#include "ast.h"
#include "parser.h"
#include "expressions.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <string.h>
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/
extern int debug;
/* --- Prototypes ---*/
void create_json(Node *node);
cJSON *node_to_json(Node *node);
cJSON *program_to_json(Node *node);
cJSON *var_decl_to_json(Node *node);
cJSON *func_decl_to_json(Node *node);
cJSON *return_stmt_to_json(Node *node);
cJSON *expr_to_json(Expr *expr);
cJSON *enum_to_json(Node *node);
cJSON *struct_to_json(Node *node);
cJSON *union_to_json(Node *node);
cJSON *while_to_json(Node *node);
cJSON *do_while_to_json(Node *node);
cJSON *for_to_json(Node *node);
cJSON *type_node_to_json(Node *node);
cJSON *if_stmt_to_json(Node *node);
cJSON *switch_to_json(Node *node);
cJSON *misc_to_json(Node *node);
cJSON *typedef_to_json(Node *node);
/* --- Main ---*/


/* --- Functions ---*/
void create_json(Node *node) {
	cJSON *root = node_to_json(node);
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

cJSON *node_to_json(Node *node) {
	switch (node->type) {
		case NODE_PROGRAM:
			return program_to_json(node);
		case NODE_VAR_DECL:
			return var_decl_to_json(node);
		case NODE_FUNC_DECL:
			return func_decl_to_json(node);
		case NODE_RETURN:
			return return_stmt_to_json(node);
		case NODE_EXPR:
			return expr_to_json(node->expr.expr);
		case NODE_ENUM_DECL:
			return enum_to_json(node);
		case NODE_STRUCT_DECL:
			return struct_to_json(node);
		case NODE_UNION_DECL:
			return union_to_json(node);
		case NODE_WHILE_STMT:
			return while_to_json(node);
		case NODE_DO_WHILE_STMT:
			return do_while_to_json(node);
		case NODE_FOR_STMT:
			return for_to_json(node);
		case NODE_TYPE:
			return type_node_to_json(node);
		case NODE_IF_STMT:
			return if_stmt_to_json(node);
		case NODE_SWITCH_STMT:
			return switch_to_json(node);
		case NODE_MISC:
			return misc_to_json(node);
		case NODE_TYPEDEF:
			return typedef_to_json(node);
		default:
			return cJSON_CreateString("<unknown node>");
	}
}

cJSON *program_to_json(Node *node) {
	cJSON *program_arr = cJSON_CreateArray();

	for (int i = 0; node->program.stmts[i]; i++) {
		Node *child = node->program.stmts[i];
		cJSON_AddItemToArray(program_arr, node_to_json(child));
	}
	cJSON *wrapper = cJSON_CreateObject();
    cJSON_AddItemToObject(wrapper, "PROGRAM", program_arr);
    return wrapper;
}

cJSON *var_decl_to_json(Node *node) {
    cJSON *var_node_arr = cJSON_CreateArray();

	cJSON *type_obj = cJSON_CreateObject();
	cJSON_AddItemToObject(type_obj, "type", node_to_json(node->var_decl.type));
    cJSON_AddItemToArray(var_node_arr, type_obj);

    // Name
    cJSON *name_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(name_obj, "name", node->var_decl.name);
    cJSON_AddItemToArray(var_node_arr, name_obj);

    // Value
	cJSON *value_obj = cJSON_CreateObject();
	if (node->var_decl.value) {
		cJSON_AddItemToObject(value_obj, "value", expr_to_json(node->var_decl.value));
	} else {
		cJSON_AddStringToObject(value_obj, "value", "<uninitialized>");
	}
	cJSON_AddItemToArray(var_node_arr, value_obj);

    // Wrap
    cJSON *wrapper = cJSON_CreateObject();
    cJSON_AddItemToObject(wrapper, "VAR_DECL", var_node_arr);
    return wrapper;
}

cJSON *func_decl_to_json(Node *node) {
	cJSON *func_node_arr = cJSON_CreateArray();

	cJSON *type_obj = cJSON_CreateObject();
	cJSON_AddItemToObject(type_obj, "type", node_to_json(node->var_decl.type));
    cJSON_AddItemToArray(func_node_arr, type_obj);

	// Name
    cJSON *name_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(name_obj, "name", node->func_decl.name);
    cJSON_AddItemToArray(func_node_arr, name_obj);

	// Arguments
	cJSON *args_arr = cJSON_CreateArray();
    for (size_t i = 0; i < node->func_decl.arg_count; i++) {
        Node *arg = node->func_decl.args[i];
        cJSON *arg_obj = node_to_json(arg);
        cJSON_AddItemToArray(args_arr, arg_obj);
    }

    cJSON *args_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(args_obj, "args", args_arr);
    cJSON_AddItemToArray(func_node_arr, args_obj);

	// Body or Prototype
	if (node->func_decl.is_prototype) {
		cJSON *proto_obj = cJSON_CreateObject();
		cJSON_AddStringToObject(proto_obj, "body", "<prototype>");
		cJSON_AddItemToArray(func_node_arr, proto_obj);
	} else {
		cJSON *body_arr = cJSON_CreateArray();
		for (size_t i = 0; i < node->func_decl.body_count; i++) {
			Node *child = node->func_decl.body[i];
			cJSON_AddItemToArray(body_arr, node_to_json(child));
		}
		cJSON *body_obj = cJSON_CreateObject();
		cJSON_AddItemToObject(body_obj, "body", body_arr);
		cJSON_AddItemToArray(func_node_arr, body_obj);
	}

	cJSON *wrapper = cJSON_CreateObject();
	cJSON_AddItemToObject(wrapper, "FUNC_DECL", func_node_arr);
	return wrapper;
}

cJSON *return_stmt_to_json(Node *node) {
	cJSON *wrapper = cJSON_CreateObject();
	cJSON_AddItemToObject(wrapper, "RETURN", expr_to_json(node->return_stmt.value));
	return wrapper;
}

cJSON *expr_to_json(Expr *expr) {
	cJSON *ex = cJSON_CreateArray();

	switch (expr->kind) {
		case EXPR_BINARY: {
			cJSON *type_obj = cJSON_CreateObject();
			cJSON_AddStringToObject(type_obj, "type", "BINARY");
			cJSON_AddItemToArray(ex, type_obj);
			cJSON *operator = cJSON_CreateObject();
            cJSON_AddStringToObject(operator, "op", expr->binary.op);
			cJSON_AddItemToArray(ex, operator);
			cJSON *left_obj = cJSON_CreateObject();
            cJSON_AddItemToObject(left_obj, "left", expr_to_json(expr->binary.left));
			cJSON_AddItemToArray(ex, left_obj);
			cJSON *right_obj = cJSON_CreateObject();
            cJSON_AddItemToObject(right_obj, "right", expr_to_json(expr->binary.right));
			cJSON_AddItemToArray(ex, right_obj);
			break;
		}

		case EXPR_UNARY: {
			cJSON *type_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(type_obj, "type", "UNARY");
			cJSON_AddItemToArray(ex, type_obj);
            cJSON *operator = cJSON_CreateObject();
            cJSON_AddStringToObject(operator, "op", expr->unary.op);
			cJSON_AddItemToArray(ex, operator);
			cJSON *operand = cJSON_CreateObject();
            cJSON_AddItemToObject(operand, "operand", expr_to_json(expr->unary.operand));
			cJSON_AddItemToArray(ex, operand);
            break;
		}

		case EXPR_LITERAL: {
			cJSON *type_obj = cJSON_CreateObject();
			cJSON_AddStringToObject(type_obj, "type", "LITERAL");
			cJSON_AddItemToArray(ex, type_obj);
			cJSON *val_obj = cJSON_CreateObject();
			cJSON_AddStringToObject(val_obj, "value", expr->literal);
			cJSON_AddItemToArray(ex, val_obj);
			break;
		}
		
		case EXPR_IDENTIFIER: {
			cJSON *type_obj = cJSON_CreateObject();
			cJSON_AddStringToObject(type_obj, "type", "IDENTIFIER");
			cJSON_AddItemToArray(ex, type_obj);
			cJSON *name_obj = cJSON_CreateObject();
			cJSON_AddStringToObject(name_obj, "name", expr->identifier);
			cJSON_AddItemToArray(ex, name_obj);
			break;
		}
		
		case EXPR_GROUPING: {
			cJSON *type_obj = cJSON_CreateObject();
			cJSON_AddStringToObject(type_obj, "type", "GROUP");
			cJSON_AddItemToArray(ex, type_obj);
			cJSON *operand = cJSON_CreateObject();
            cJSON_AddItemToObject(operand, "inner", expr_to_json(expr->group.expr));
			cJSON_AddItemToArray(ex, operand);
			//cJSON_AddItemToObject(ex, "expr", expr_to_json(expr->group.expr));
			break;
		}
		case EXPR_CALL: {
			cJSON *type_obj = cJSON_CreateObject();
			cJSON_AddStringToObject(type_obj, "type", "CALL");
			cJSON_AddItemToArray(ex, type_obj);

			cJSON *func_obj = cJSON_CreateObject();
			cJSON_AddStringToObject(func_obj, "func", expr->call.func);
			cJSON_AddItemToArray(ex, func_obj);

			cJSON *arg_arr = cJSON_CreateArray();
			for (size_t i = 0; i < expr->call.arg_count; i++) {
				Expr *arg = expr->call.args[i];
				cJSON *arg_obj = expr_to_json(arg);
				cJSON_AddItemToArray(arg_arr, arg_obj);
			}
			cJSON *args_obj = cJSON_CreateObject();
			cJSON_AddItemToObject(args_obj, "args", arg_arr);
			cJSON_AddItemToArray(ex, args_obj);
			break;
		}
	}
	cJSON *wrapper = cJSON_CreateObject();
	cJSON_AddItemToObject(wrapper, "EXPRESSION", ex);
	return wrapper;
}

cJSON *enum_to_json(Node *node) {
	cJSON *enum_arr = cJSON_CreateArray();

	cJSON *name_obj = cJSON_CreateObject();
	if (node->enum_decl.name) {
		cJSON_AddStringToObject(name_obj, "name", node->enum_decl.name);
	} else {
		cJSON_AddNullToObject(name_obj, "name");
	}
	cJSON_AddItemToArray(enum_arr, name_obj);

	cJSON *members_arr = cJSON_CreateArray();
	for (size_t i = 0; i < node->enum_decl.member_count; i++) {
		EnumMember *member = &node->enum_decl.members[i];

		cJSON *member_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(member_obj, "name", member->name);

        if (member->value) {
            cJSON *value_expr = expr_to_json(member->value);
			cJSON_AddItemToArray(members_arr, value_expr);
        } else {
			cJSON_AddNullToObject(member_obj, "value");
		}
		cJSON_AddItemToArray(members_arr, member_obj);
    }
	cJSON *members_obj = cJSON_CreateObject();
	cJSON_AddItemToObject(members_obj, "members", members_arr);
	cJSON_AddItemToArray(enum_arr, members_obj);
	cJSON *wrapper = cJSON_CreateObject();
	cJSON_AddItemToObject(wrapper, "ENUM", enum_arr);
	return wrapper;
}

cJSON *struct_to_json(Node *node) {
	cJSON *struct_arr = cJSON_CreateArray();

	cJSON *name_obj = cJSON_CreateObject();
	if (node->struct_decl.name) {
		cJSON_AddStringToObject(name_obj, "name", node->struct_decl.name);
	} else {
		cJSON_AddNullToObject(name_obj, "name");
	}
	cJSON_AddItemToArray(struct_arr, name_obj);	

	cJSON *args_arr = cJSON_CreateArray();
    for (size_t i = 0; i < node->struct_decl.member_count; i++) {
        Node *arg = node->struct_decl.members[i];
        cJSON *arg_obj = node_to_json(arg);
		cJSON_AddItemToArray(args_arr, arg_obj);
    }

    cJSON *args_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(args_obj, "args", args_arr);
    cJSON_AddItemToArray(struct_arr, args_obj);

	cJSON *wrapper = cJSON_CreateObject();
	cJSON_AddItemToObject(wrapper, "STRUCT", struct_arr);
	return wrapper;
}

cJSON *union_to_json(Node *node) {
	cJSON *union_arr = cJSON_CreateArray();

	cJSON *name_obj = cJSON_CreateObject();
	if (node->union_decl.name) {
		cJSON_AddStringToObject(name_obj, "name", node->union_decl.name);
	} else {
		cJSON_AddNullToObject(name_obj, "name");
	}
	cJSON_AddItemToArray(union_arr, name_obj);	

	cJSON *args_arr = cJSON_CreateArray();
    for (size_t i = 0; i < node->union_decl.member_count; i++) {
        Node *arg = node->union_decl.members[i];
        cJSON *arg_obj = node_to_json(arg);
		cJSON_AddItemToArray(args_arr, arg_obj);
    }

    cJSON *args_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(args_obj, "args", args_arr);
    cJSON_AddItemToArray(union_arr, args_obj);

	cJSON *wrapper = cJSON_CreateObject();
	cJSON_AddItemToObject(wrapper, "UNION", union_arr);
	return wrapper;
}

cJSON *while_to_json(Node *node) {
	cJSON *while_arr = cJSON_CreateArray();

	cJSON *cond_obj = cJSON_CreateObject();
	cJSON_AddItemToObject(cond_obj, "condition", expr_to_json(node->while_stmt.cond));
	cJSON_AddItemToArray(while_arr, cond_obj);

	cJSON *body_arr = cJSON_CreateArray();
	for (size_t i = 0; node->while_stmt.body[i]; i++) {
		Node *child = node->while_stmt.body[i];
		cJSON_AddItemToArray(body_arr, node_to_json(child));
	}
	cJSON *body_obj = cJSON_CreateObject();
	cJSON_AddItemToObject(body_obj, "body", body_arr);
	cJSON_AddItemToArray(while_arr, body_obj);

	cJSON *wrapper = cJSON_CreateObject();
	cJSON_AddItemToObject(wrapper, "WHILE", while_arr);
	return wrapper;
}

cJSON *do_while_to_json(Node *node) {
	cJSON *do_while_arr = cJSON_CreateArray();

	cJSON *body_arr = cJSON_CreateArray();
	for (size_t i = 0; node->do_while_stmt.body[i]; i++) {
		Node *child = node->do_while_stmt.body[i];
		cJSON_AddItemToArray(body_arr, node_to_json(child));
	}
	cJSON *body_obj = cJSON_CreateObject();
	cJSON_AddItemToObject(body_obj, "body", body_arr);
	cJSON_AddItemToArray(do_while_arr, body_obj);

	cJSON *cond_obj = cJSON_CreateObject();
	cJSON_AddItemToObject(cond_obj, "condition", expr_to_json(node->do_while_stmt.cond));
	cJSON_AddItemToArray(do_while_arr, cond_obj);

	cJSON *wrapper = cJSON_CreateObject();
	cJSON_AddItemToObject(wrapper, "DO-WHILE", do_while_arr);
	return wrapper;
}

cJSON *for_to_json(Node *node) {
	cJSON *for_arr = cJSON_CreateArray();

	cJSON *init_obj = cJSON_CreateObject();
	if (node->for_stmt.init) {
		cJSON_AddItemToObject(init_obj, "initialiser", node_to_json(node->for_stmt.init));
	} else {
		cJSON_AddStringToObject(init_obj, "initializer", "<none>");
	}
	cJSON_AddItemToArray(for_arr, init_obj);

	cJSON *cond_obj = cJSON_CreateObject();
	if (node->for_stmt.cond) {
		cJSON_AddItemToObject(cond_obj, "condition", node_to_json(node->for_stmt.cond));
	} else {
		cJSON_AddStringToObject(cond_obj, "condition", "<none>");	
	}
	cJSON_AddItemToArray(for_arr, cond_obj);

	cJSON *inc_obj = cJSON_CreateObject();
	if (node->for_stmt.inc) {
		cJSON_AddItemToObject(inc_obj, "increment", expr_to_json(node->for_stmt.inc));
	} else {
		cJSON_AddStringToObject(inc_obj, "increment", "<none>");
	}
	cJSON_AddItemToArray(for_arr, inc_obj);

	cJSON *body_arr = cJSON_CreateArray();
	for (size_t i = 0; node->for_stmt.body[i]; i++) {
		Node *child = node->for_stmt.body[i];
		cJSON_AddItemToArray(body_arr, node_to_json(child));
	}
	cJSON *body_obj = cJSON_CreateObject();
	cJSON_AddItemToObject(body_obj, "body", body_arr);
	cJSON_AddItemToArray(for_arr, body_obj);


	cJSON *wrapper = cJSON_CreateObject();
	cJSON_AddItemToObject(wrapper, "FOR", for_arr);
	return wrapper;
}

cJSON *type_node_to_json(Node *node) {
	cJSON *type_arr = cJSON_CreateArray();

	cJSON *spec_obj = cJSON_CreateObject();
	if (node->type_node.spec) {
		TypeSpec *s = node->type_node.spec;

		const char *storage_s = "none";
		switch (s->storage) {
			case AUTO: storage_s = "auto"; break;
			case REGISTER: storage_s = "register"; break;
			case STATIC: storage_s = "static"; break;
			case EXTERN: storage_s = "extern"; break;
			default: break;
		}
		cJSON_AddStringToObject(spec_obj, "storage", storage_s);

		const char *sign_s = "none";
		switch (s->sign) {
			case SIGNED_T: sign_s = "signed"; break;
			case UNSIGNED_T: sign_s = "unsigned"; break;
			default: break;
		}
		cJSON_AddStringToObject(spec_obj, "sign", sign_s);

		const char *len_s = "none";
		switch (s->length) {
			case SHORT_T: len_s = "short"; break;
			case LONG_T: len_s = "long"; break;
			case LONGLONG_T: len_s = "long long"; break;
			default: break;
		}
		cJSON_AddStringToObject(spec_obj, "length", len_s);

		cJSON_AddBoolToObject(spec_obj, "const", s->is_const);
        cJSON_AddBoolToObject(spec_obj, "volatile", s->is_volatile);
        cJSON_AddBoolToObject(spec_obj, "inline", s->is_inline);
        cJSON_AddBoolToObject(spec_obj, "restrict", s->is_restrict);
        cJSON_AddNumberToObject(spec_obj, "pointer_depth", s->pointer_depth);

	} else {
		cJSON_AddStringToObject(spec_obj, "spec", "<none>");
	}

	cJSON_AddItemToArray(type_arr, spec_obj);

	if (node->type_node.is_decl) {
        /* nested decl (struct/enum) */
        if (node->type_node.decl) {
            cJSON *decl_json = node_to_json(node->type_node.decl);
            cJSON_AddItemToArray(type_arr, decl_json);
        } else {
            cJSON *d = cJSON_CreateObject();
            cJSON_AddNullToObject(d, "decl");
            cJSON_AddItemToArray(type_arr, d);
        }
    } else {
        cJSON *base_obj = cJSON_CreateObject();
        if (node->type_node.base) {
            cJSON_AddStringToObject(base_obj, "base", node->type_node.base);
        } else {
            cJSON_AddStringToObject(base_obj, "base", "int");
        }
        cJSON_AddItemToArray(type_arr, base_obj);
    }
    
    cJSON *wrapper = cJSON_CreateObject();
	cJSON_AddItemToObject(wrapper, "TYPE", type_arr);
	return wrapper;
}

cJSON *if_stmt_to_json(Node *node) {
    cJSON *if_arr = cJSON_CreateArray();

    // IF condition
    cJSON *if_cond_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(if_cond_obj, "condition", expr_to_json(node->if_stmt.if_cond));
    cJSON_AddItemToArray(if_arr, if_cond_obj);

    // IF body
    cJSON *if_body_arr = cJSON_CreateArray();
    if (node->if_stmt.if_body) {
        for (size_t i = 0; node->if_stmt.if_body[i]; i++) {
            cJSON_AddItemToArray(if_body_arr, node_to_json(node->if_stmt.if_body[i]));
        }
    }
    cJSON *if_body_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(if_body_obj, "body", if_body_arr);
    cJSON_AddItemToArray(if_arr, if_body_obj);

    // ELSE IF blocks
    if (node->if_stmt.elif_count > 0) {
        for (size_t i = 0; i < node->if_stmt.elif_count; i++) {
			cJSON *elif_arr = cJSON_CreateArray();
            cJSON *elif_obj = cJSON_CreateObject();
            cJSON_AddItemToObject(elif_obj, "condition", expr_to_json(node->if_stmt.elif_conds[i]));

            cJSON *elif_body_arr = cJSON_CreateArray();
            if (node->if_stmt.elif_bodies[i]) {
                for (size_t j = 0; node->if_stmt.elif_bodies[i][j]; j++) {
                    cJSON_AddItemToArray(elif_body_arr, node_to_json(node->if_stmt.elif_bodies[i][j]));
                }
            }
            cJSON_AddItemToObject(elif_obj, "body", elif_body_arr);
            cJSON_AddItemToArray(elif_arr, elif_obj);

			cJSON *elif_wrapper = cJSON_CreateObject();
			cJSON_AddItemToObject(elif_wrapper, "else if", elif_arr);
			cJSON_AddItemToArray(if_arr, elif_wrapper);
        }
    }

    // ELSE block
    if (node->if_stmt.else_body) {
        cJSON *else_body_arr = cJSON_CreateArray();
        for (size_t i = 0; node->if_stmt.else_body[i]; i++) {
            cJSON_AddItemToArray(else_body_arr, node_to_json(node->if_stmt.else_body[i]));
        }
        cJSON *else_body_obj = cJSON_CreateObject();
        cJSON_AddItemToObject(else_body_obj, "else body", else_body_arr);
        cJSON_AddItemToArray(if_arr, else_body_obj);
    }

    cJSON *wrapper = cJSON_CreateObject();
    cJSON_AddItemToObject(wrapper, "IF", if_arr);
    return wrapper;
}

cJSON *switch_to_json(Node *node) {
	cJSON *switch_arr = cJSON_CreateArray();

	cJSON *expr_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(expr_obj, "expression", expr_to_json(node->switch_stmt.expression));
    cJSON_AddItemToArray(switch_arr, expr_obj);

	if (node->switch_stmt.case_count > 0) {
        for (size_t i = 0; i < node->switch_stmt.case_count; i++) {
			cJSON *case_arr = cJSON_CreateArray();
            cJSON *case_obj = cJSON_CreateObject();
            cJSON_AddItemToObject(case_obj, "condition", expr_to_json(node->switch_stmt.cases[i]));

            cJSON *case_body_arr = cJSON_CreateArray();
            if (node->switch_stmt.case_bodies[i]) {
                for (size_t j = 0; node->switch_stmt.case_bodies[i][j]; j++) {
                    cJSON_AddItemToArray(case_body_arr, node_to_json(node->switch_stmt.case_bodies[i][j]));
                }
            }
            cJSON_AddItemToObject(case_obj, "body", case_body_arr);
            cJSON_AddItemToArray(case_arr, case_obj);

			cJSON *case_wrapper = cJSON_CreateObject();
			cJSON_AddItemToObject(case_wrapper, "case", case_arr);
			cJSON_AddItemToArray(switch_arr, case_wrapper);
        }
    }

	if (node->switch_stmt.default_body) {
        cJSON *default_body_arr = cJSON_CreateArray();
        for (size_t i = 0; node->switch_stmt.default_body[i]; i++) {
            cJSON_AddItemToArray(default_body_arr, node_to_json(node->switch_stmt.default_body[i]));
        }
        cJSON *default_body_obj = cJSON_CreateObject();
        cJSON_AddItemToObject(default_body_obj, "default", default_body_arr);
        cJSON_AddItemToArray(switch_arr, default_body_obj);
    }

	cJSON *wrapper = cJSON_CreateObject();
    cJSON_AddItemToObject(wrapper, "SWITCH", switch_arr);
	return wrapper;
}

cJSON *misc_to_json(Node *node) {
	cJSON *wrapper = cJSON_CreateObject();
    cJSON_AddStringToObject(wrapper, "MISC", node->misc.name);
	return wrapper;
}

cJSON *typedef_to_json(Node *node) {
	cJSON *typedef_arr = cJSON_CreateArray();

	cJSON *name_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(name_obj, "name", node->typedef_node.name);
    cJSON_AddItemToArray(typedef_arr, name_obj);

	cJSON *type_obj = cJSON_CreateObject();
	cJSON *type_arr = cJSON_CreateArray();

	cJSON_AddItemToArray(type_arr, node_to_json(node->typedef_node.type));
	cJSON_AddItemToObject(type_obj, "type", type_arr);

	cJSON_AddItemToArray(typedef_arr, type_obj);

	cJSON *wrapper = cJSON_CreateObject();
    cJSON_AddItemToObject(wrapper, "TYPEDEF", typedef_arr);
	return wrapper;
}