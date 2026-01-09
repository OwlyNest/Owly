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
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/
extern int debug;
/* --- Prototypes ---*/
void emit_symbol_table(SemanticContext *ctx);
cJSON *emit_scope_json(Scope *scope);
cJSON *emit_symbol_json(Symbol *sym);

/* --- Functions ---*/
void emit_symbol_table(SemanticContext *ctx) {
    cJSON *root = cJSON_CreateObject();
    
    cJSON *global = emit_scope_json(ctx->global_scope);
    cJSON_AddItemToObject(root, "global_scope", global);
    
    cJSON_AddNumberToObject(root, "errors", ctx->error_count);
    cJSON_AddNumberToObject(root, "warnings", ctx->warning_count);
    
    char *json_str = cJSON_Print(root);
    FILE *f = fopen("symbols.json", "w");
    fprintf(f, "%s", json_str);
    fclose(f);
	if (debug) {
        printf("%s\n", json_str);
    }
    
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
    if (sym->kind == SYM_VARIABLE) {
        cJSON_AddNumberToObject(sym_obj, "initialised", sym->variable.is_initialized);
    }
    
    // TODO: Add type info, initialization status, etc.
    
    return sym_obj;
}