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

#include <cjson/cJSON.h>
#include <stdio.h>
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/

/* --- Prototypes ---*/
cJSON *var_decl_to_json(Node *node);
cJSON *func_decl_to_json(Node *node);
cJSON *return_stmt_to_json(Node *node);
/* --- Main ---*/


/* --- Functions ---*/
cJSON *node_to_json(Node *node) {
	switch (node->type) {
		case NODE_VAR_DECL:
			return var_decl_to_json(node);
		case NODE_FUNC_DECL:
			return func_decl_to_json(node);
		case NODE_RETURN:
			return return_stmt_to_json(node);
		default:
			return cJSON_CreateString("<unknown node>");
	}
}

cJSON *var_decl_to_json(Node *node) {
    cJSON *var_node_arr = cJSON_CreateArray();

    // Properties
    cJSON *props_obj = cJSON_CreateObject();
    cJSON *props_arr = cJSON_CreateArray();
    for (int i = 0; node->var_decl.properties[i] != NULL; i++) {
        cJSON_AddItemToArray(props_arr, cJSON_CreateString(node->var_decl.properties[i]));
    }
    cJSON_AddItemToObject(props_obj, "properties", props_arr);
    cJSON_AddItemToArray(var_node_arr, props_obj);

    // Type
    char typebuf[64];
    snprintf(typebuf, sizeof(typebuf), "%s%s",
        node->var_decl.type,
        node->var_decl.is_pointer ? "*" : "");
    cJSON *type_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(type_obj, "type", typebuf);
    cJSON_AddItemToArray(var_node_arr, type_obj);

    // Name
    cJSON *name_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(name_obj, "name", node->var_decl.name);
    cJSON_AddItemToArray(var_node_arr, name_obj);

    // Value
    cJSON *value_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(value_obj, "value",
        node->var_decl.value ? node->var_decl.value : "<uninitialized>");
    cJSON_AddItemToArray(var_node_arr, value_obj);

    // Wrap
    cJSON *wrapper = cJSON_CreateObject();
    cJSON_AddItemToObject(wrapper, "VAR_DECL", var_node_arr);
    return wrapper;
}

cJSON *func_decl_to_json(Node *node) {
	cJSON *func_node_arr = cJSON_CreateArray();
	
	// Properties
	cJSON *props_obj = cJSON_CreateObject();
	cJSON *props_arr = cJSON_CreateArray();

	for (int i = 0; node->func_decl.properties[i] != NULL; i++) {
		cJSON_AddItemToArray(props_arr, cJSON_CreateString(node->func_decl.properties[i]));
	}
	cJSON_AddItemToObject(props_obj, "properties", props_arr);
	cJSON_AddItemToArray(func_node_arr, props_obj);

	// Type
	char typebuf[64];
	snprintf(typebuf, sizeof(typebuf), "%s%s",
        node->func_decl.type,
        node->func_decl.is_pointer ? "*" : "");
	cJSON *type_obj = cJSON_CreateObject();
	cJSON_AddStringToObject(type_obj, "type", typebuf);
	cJSON_AddItemToArray(func_node_arr, type_obj);

	// Name
    cJSON *name_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(name_obj, "name", node->func_decl.name);
    cJSON_AddItemToArray(func_node_arr, name_obj);

	// Arguments
	cJSON *args_arr = cJSON_CreateArray();
    for (size_t i = 0; i < node->func_decl.arg_count; i++) {
        Node *arg = node->func_decl.args[i];
        cJSON *arg_obj = cJSON_CreateObject();

        char arg_typebuf[64];
        snprintf(arg_typebuf, sizeof(arg_typebuf), "%s%s",
            arg->var_decl.type,
            arg->var_decl.is_pointer ? "*" : "");

        cJSON_AddStringToObject(arg_obj, "type", arg_typebuf);
        cJSON_AddStringToObject(arg_obj, "name", arg->var_decl.name);
        cJSON_AddItemToArray(args_arr, arg_obj);
    }

    cJSON *args_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(args_obj, "args", args_arr);
    cJSON_AddItemToArray(func_node_arr, args_obj);

	// Body or Prototype
	if (node->func_decl.is_prototype) {
		cJSON *proto_obj = cJSON_CreateObject();
		cJSON_AddStringToObject(proto_obj, "body", "<prototype");
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
	cJSON_AddStringToObject(wrapper, "RETURN", node->return_stmt.value);
	return wrapper;
}