/*
	* SA_to_json.c - [Enter description]
	* Author:   Amity
	* Date:     Wed Dec 24 00:14:17 2025
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
#include "front/ast_to_json.h"
#include <cjson/cJSON.h>
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/
/* --- Prototypes ---*/
void emit_symbol_table(SemanticContext *ctx);
cJSON *emit_scope_json(Scope *scope);
cJSON *emit_symbol_json(Symbol *sym);
cJSON *resolved_type_to_json(ResolvedType *rt);

/* --- Functions ---*/
void emit_symbol_table(SemanticContext *ctx) {
    cJSON *root = cJSON_CreateObject();
    
    cJSON *global = emit_scope_json(ctx->global_scope);
    cJSON_AddItemToObject(root, "global_scope", global);
    
    cJSON_AddNumberToObject(root, "errors", ctx->error_count);
    cJSON_AddNumberToObject(root, "warnings", ctx->warning_count);
    
    char *json_str = cJSON_Print(root);
    FILE *f = fopen("out/symbols.json", "w");
    fprintf(f, "%s", json_str);
    fclose(f);
    
    free(json_str);
    cJSON_Delete(root);
}

cJSON *emit_scope_json(Scope *scope) {
    cJSON *scope_obj = cJSON_CreateObject();
    
    const char *scope_type_str;
    switch (scope->type) {
        case SCOPE_GLOBAL:   scope_type_str = "global";   break;
        case SCOPE_FUNCTION: scope_type_str = "function"; break;
        case SCOPE_BLOCK:    scope_type_str = "block";    break;
        case SCOPE_STRUCT:   scope_type_str = "struct";   break;
        case SCOPE_ENUM:     scope_type_str = "enum";     break;
        case SCOPE_UNION:    scope_type_str = "union";    break;
        default:             scope_type_str = "unknown";  break;
    }
    cJSON_AddStringToObject(scope_obj, "type", scope_type_str);
    
    if (scope->name) {
        cJSON_AddStringToObject(scope_obj, "name", scope->name);
    }
    
    // Add symbols array
    cJSON *symbols_array = cJSON_CreateArray();
    for (size_t i = 0; i < scope->symbol_count; i++) {
        cJSON *sym_obj = emit_symbol_json(scope->symbols[i]);
        cJSON_AddItemToArray(symbols_array, sym_obj);
    }
    cJSON_AddItemToObject(scope_obj, "symbols", symbols_array);
    
    // Add child scopes
    if (scope->child_count > 0) {
        cJSON *children_array = cJSON_CreateArray();
        for (size_t i = 0; i < scope->child_count; i++) {
            cJSON *child_obj = emit_scope_json(scope->children[i]);
            cJSON_AddItemToArray(children_array, child_obj);
        }
        cJSON_AddItemToObject(scope_obj, "child_scopes", children_array);
    }
    
    return scope_obj;
}

cJSON *emit_symbol_json(Symbol *sym) {
    cJSON *sym_obj = cJSON_CreateObject();
    
    // Basic symbol info
    cJSON_AddStringToObject(sym_obj, "name", sym->name);
    
    const char *kind_str;
    switch (sym->kind) {
        case SYM_VARIABLE:    kind_str = "variable";    break;
        case SYM_FUNCTION:    kind_str = "function";    break;
        case SYM_TYPEDEF:     kind_str = "typedef";     break;
        case SYM_ENUM:        kind_str = "enum";        break;
        case SYM_STRUCT:      kind_str = "struct";      break;
        case SYM_UNION:       kind_str = "union";       break;
        case SYM_ENUM_MEMBER: kind_str = "enum_member"; break;
        default:              kind_str = "unknown";     break;
    }
    cJSON_AddStringToObject(sym_obj, "kind", kind_str);

    // Variable info
    if (sym->kind == SYM_VARIABLE) {
        cJSON_AddNumberToObject(sym_obj, "initialised", sym->variable.is_initialized);
        if (sym->variable.type) {
            cJSON_AddItemToObject(
                sym_obj,
                "type",
                type_node_to_json(sym->variable.type)
            );
        }
    }

    // Function info
    if (sym->kind == SYM_FUNCTION) {
        if (sym->function.return_type) {
            cJSON_AddItemToObject(
                sym_obj,
                "return_type",
                type_node_to_json(sym->function.return_type)
            );
        }
    
        cJSON *params = cJSON_CreateArray();
        for (size_t i = 0; i < sym->function.param_count; i++) {
            Node *param = sym->function.params[i];
            if (param && param->var_decl.type) {
                cJSON_AddItemToArray(
                    params,
                    type_node_to_json(param->var_decl.type)
                );
            }
        }
        cJSON_AddItemToObject(sym_obj, "params", params);
    }
    

    // Typedef info
    if (sym->kind == SYM_TYPEDEF) {
        if (sym->typedef_sym.actual_type) {
            cJSON_AddItemToObject(
                sym_obj,
                "actual_type",
                type_node_to_json(sym->typedef_sym.actual_type)
            );
        }
    }

    // Enum members (optional)
    if (sym->kind == SYM_ENUM) {
        cJSON *members = cJSON_CreateArray();
        for (size_t i = 0; i < sym->enum_sym.member_count; i++)
            cJSON_AddItemToArray(members, cJSON_CreateString(sym->enum_sym.members[i].name));
        cJSON_AddItemToObject(sym_obj, "members", members);
    }

    if (sym->resolved) {
        cJSON_AddItemToObject(sym_obj,
            "resolved type",
            resolved_type_to_json(sym->resolved)
        );
    }

    cJSON *wrapper = cJSON_CreateObject();
    cJSON_AddItemToObject(wrapper, sym->name, sym_obj);
    return wrapper;
}

cJSON *resolved_type_to_json(ResolvedType *rt) {
    if (!rt) return cJSON_CreateNull();

    cJSON *obj = cJSON_CreateObject();

    /* Kind */
    const char *kind_str;
    switch (rt->kind) {
        case RT_INVALID:   kind_str = "invalid";   break;
        case RT_BUILTIN:   kind_str = "builtin";   break;
        case RT_POINTER:   kind_str = "pointer";   break;
        case RT_STRUCT:    kind_str = "struct";    break;
        case RT_UNION:     kind_str = "union";     break;
        case RT_ENUM:      kind_str = "enum";      break;
        case RT_FUNCTION:  kind_str = "function";  break;
        case RT_ARRAY:     kind_str = "array"; break;
        default:           kind_str = "unknown";   break;
    }
    cJSON_AddStringToObject(obj, "kind", kind_str);

    /* Name */
    if (rt->name) {
        cJSON_AddStringToObject(obj, "name", rt->name);
    }

    /* Qualifiers */
    if (rt->is_const) {
        cJSON_AddBoolToObject(obj, "const", 1);
    }
    if (rt->is_volatile) {
        cJSON_AddBoolToObject(obj, "volatile", 1);
    }

    /* Size and alignment */
    if (rt->size > 0) {
        cJSON_AddNumberToObject(obj, "size", (double)rt->size);
    }
    if (rt->align > 0) {
        cJSON_AddNumberToObject(obj, "align", (double)rt->align);
    }

    /* Builtin properties */
    if (rt->kind == RT_BUILTIN) {
        cJSON_AddBoolToObject(obj, "signed", rt->is_signed);
        cJSON_AddBoolToObject(obj, "floating", rt->is_floating);
    }

    /* Completeness */
    if (rt->kind == RT_STRUCT || rt->kind == RT_UNION) {
        cJSON_AddBoolToObject(obj, "complete", rt->is_complete);
    }

    /* Base type (for pointers and functions) */
    if (rt->base) {
        cJSON_AddItemToObject(obj, "base", 
            resolved_type_to_json(rt->base)
        );
    }

    /* Parameters (for functions) */
    if (rt->kind == RT_FUNCTION && rt->param_count > 0) {
        cJSON *params_arr = cJSON_CreateArray();
        for (size_t i = 0; i < rt->param_count; i++) {
            if (rt->params && rt->params[i]) {
                cJSON_AddItemToArray(params_arr,
                    resolved_type_to_json(rt->params[i])
                );
            } else {
                cJSON_AddItemToArray(params_arr, cJSON_CreateNull());
            }
        }
        cJSON_AddItemToObject(obj, "params", params_arr);
    }

    if (rt->kind == RT_FUNCTION && rt->is_variadic) {
        cJSON_AddBoolToObject(obj, "variadic", 1);
    }

    if (rt->kind == RT_ENUM && rt->enum_base) {
        cJSON_AddItemToObject(obj, "enum_base",
            resolved_type_to_json(rt->enum_base)
        );
    }

    if (rt->kind == RT_ARRAY) {
        cJSON *dims = cJSON_CreateArray();
        for (size_t i = 0; i < rt->dim_count; i++) {
            cJSON_AddItemToArray(dims, cJSON_CreateNumber(rt->dimensions[i]));
        }
        cJSON_AddItemToObject(obj, "dimensions", dims);
        cJSON_AddNumberToObject(obj, "total_elements", rt->total_elements);
    }

    return obj;
}