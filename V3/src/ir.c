/*
	* src/ir.c - [Enter description]
	* Author:   Amity
	* Date:     Wed Feb  4 01:04:12 2026
	* Copyright © 2026 OwlyNest
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
#include "ir.h"
#include "SA.h"
#include "front/ast.h"
#include "front/expressions.h"
#include "memutils.h"

#include <cjson/cJSON.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/
extern const char* name;
/* --- Prototypes ---*/
IRModule *generate_ir(Node *ast, SemanticContext *ctx);

void ir_free_value(IRValue *val);
void ir_free_instr(IRInstr *instr);
void ir_free_block(IRBasicBlock *block);
void ir_free_function(IRFunction *func);
void ir_free_module(IRModule *module);

const char *op_to_string(IROp op);
cJSON *value_to_json(IRValue *val);
cJSON *instr_to_json(IRInstr *instr);
cJSON *block_to_json(IRBasicBlock *block);
cJSON *function_to_json(IRFunction *func);
void ir_emit_json(IRModule *module, const char *filename);
void ir_emit_text(IRModule *module, const char *filename);
void print_value(FILE *f, IRValue *val);

/* Construction */
IRModule *ir_create_module(const char *source_file);
IRFunction *ir_create_function(IRModule *mod, const char *name, ResolvedType *return_type);
IRBasicBlock *ir_create_block(IRFunction *func, const char *name);
IRValue *ir_create_temp(IRFunction *func, ResolvedType *type);
IRValue *ir_create_const_int(long long value, ResolvedType *type);
IRValue *ir_create_const_float(double value, ResolvedType *type);
IRValue *ir_create_const_string(const char *value);
IRValue *ir_create_global(const char *name, ResolvedType *type);
IRValue *ir_create_label(IRFunction *func);

/* Emission */
IRInstr *ir_emit(IRBasicBlock *block, IROp op, IRValue *dest);
void ir_set_args(IRInstr *instr, IRValue *arg1, IRValue *arg2);
void ir_set_call(IRInstr *instr, IRValue *callee, IRValue **args, size_t arg_count);
void ir_set_branch(IRInstr *instr, IRValue *cond, IRBasicBlock *true_block, IRBasicBlock *false_block);
void ir_set_jump(IRInstr *instr, IRBasicBlock *target);
void ir_set_return(IRInstr *instr, IRValue *ret_value);

void lower_stmt(IRFunction *func, IRBasicBlock **current_block, Node *stmt, IRModule *mod, LoopContext *loop_ctx);
IRValue *lower_expr(IRFunction *func, IRBasicBlock **block, Expr *expr, IRModule *mod, int is_lvalue);
IROp determine_cast_op(ResolvedType *source, ResolvedType *target);
void flatten_set(Expr *set, Expr ***out_elements, size_t *out_count);

/* --- Functions ---*/
IRModule *generate_ir(Node *ast, SemanticContext *ctx) {
	if (!ast || ast->type != NODE_PROGRAM) return NULL;

	IRModule *mod = ir_create_module(name);

    for (size_t i = 0; ast->program.stmts[i]; i++) {
        Node *stmt = ast->program.stmts[i];

        switch (stmt->type) {
            case NODE_VAR_DECL: {
                IRValue *global = ir_create_global(stmt->var_decl.name, stmt->rtype);
                mod->globals = xrealloc(mod->globals, sizeof(IRValue *) * (mod->global_count + 1));
                mod->globals[mod->global_count++] = global;
                break;
            }
            case NODE_FUNC_DECL: {
                IRFunction *func = ir_create_function(mod, stmt->func_decl.name, stmt->rtype);
                IRBasicBlock *entry = ir_create_block(func, "entry");
                size_t a_count = stmt->func_decl.arg_count;
                if (a_count > 0) {
                    func->params = xcalloc(a_count, sizeof(IRValue *));
                    func->param_count = a_count;

                    for (size_t i = 0; i < a_count; i++) {
                        Node *param = stmt->func_decl.args[i];

                        IRValue *param_val = ir_create_temp(func, param->rtype);
                        func->params[i] = param_val;

                        IRValue *param_ptr = ir_create_temp(func, param->rtype);
                        IRInstr *alloca = ir_emit(entry, IR_ALLOCA, param_ptr);
                        alloca->alloca.size = param->rtype ? param->rtype->size : 4;
                        alloca->alloca.align = param->rtype ? param->rtype->align : 4;

                        IRInstr *store = ir_emit(entry, IR_STORE, NULL);
                        ir_set_args(store, param_val, param_ptr);

                        func->var_map = xrealloc(func->var_map, sizeof(VarMapping) * (func->var_map_count + 1));
                        func->var_map[func->var_map_count].name = strdup(param->var_decl.name);
                        func->var_map[func->var_map_count].value = param_ptr;
                        func->var_map_count++;
                    }
                }

                IRBasicBlock *current = entry;
                for (size_t j = 0; j < stmt->func_decl.body_count; j++) {
                    lower_stmt(func, &current, stmt->func_decl.body[j], mod, NULL);
                }

                if (current && (!current->last || (current->last->op != IR_RETURN && current->last->op != IR_BRANCH && current->last->op != IR_JUMP))) {
                    ir_emit(current, IR_RETURN, NULL);
                }
                break;
            }
            default:
                break;
        }
    }

	ir_emit_json(mod, "out/ir.json");
    ir_emit_text(mod, "out/ir.ir");
    
    (void)ctx;
    return mod;
}

/* Freeing */
void ir_free_value(IRValue *val) {
	if (!val) return;

	if (val->kind == IR_VALUE_GLOBAL && val->name) {
		xfree(val->name);
	} else if (val->kind == IR_VALUE_CONST_STRING && val->string_val) {
		xfree(val->string_val);
	}
	xfree(val);
}

void ir_free_instr(IRInstr *instr) {
	if (!instr) return;
	switch (instr->op) {
		case IR_CALL:
            if (instr->call.args) {
                xfree(instr->call.args);
            }
            break;
            
        case IR_PHI:
            if (instr->phi.incoming) {
                xfree(instr->phi.incoming);
            }
            break;
            
        default:
            break;
	}
	xfree(instr);
}

void ir_free_block(IRBasicBlock *block) {
	if (!block) return;

	IRInstr *instr = block->first;
    while (instr) {
        IRInstr *next = instr->next;
        ir_free_instr(instr);
        instr = next;
    }

    if (block->preds) {
        xfree(block->preds);
    }
    if (block->succs) {
        xfree(block->succs);
    }

    if (block->name) {
        xfree(block->name);
    }
    
    xfree(block);

}

void ir_free_function(IRFunction *func) {
	if (!func) return;

	if (func->name) {
        xfree(func->name);
    }

	IRBasicBlock *block = func->blocks;
    while (block) {
        IRBasicBlock *next = block->next;
        ir_free_block(block);
        block = next;
    }

	if (func->params) {
        xfree(func->params);
    }

	for (size_t i = 0; i < func->value_count; i++) {
        ir_free_value(func->values[i]);
    }
    if (func->values) {
        xfree(func->values);
    }

    for (size_t i = 0; i < func->var_map_count; i++) {
        xfree(func->var_map[i].name);
    }
    if (func->var_map) {
        xfree(func->var_map);
    }

	xfree(func);
}

void ir_free_module(IRModule *module) {
	if (!module) return;

	IRFunction *func = module->functions;
    while (func) {
        IRFunction *next = func->next;
        ir_free_function(func);
        func = next;
    }

	if (module->globals) {
        xfree(module->globals);
    }

	for (size_t i = 0; i < module->constant_count; i++) {
        ir_free_value(module->constants[i]);
    }
    if (module->constants) {
        xfree(module->constants);
    }

	if (module->source_file) {
        xfree((char *)module->source_file);
    }
    
    xfree(module);

	printf("[X] Freed IR successfully\n");
}
/* --- JSON Emission --- */

const char *op_to_string(IROp op) {
    switch (op) {
        case IR_ALLOCA: return "alloca";
        case IR_LOAD: return "load";
        case IR_STORE: return "store";
        case IR_ADD: return "add";
        case IR_SUB: return "sub";
        case IR_MUL: return "mul";
        case IR_SDIV: return "sdiv";
        case IR_UDIV: return "udiv";
        case IR_SMOD: return "smod";
        case IR_UMOD: return "umod";
        case IR_AND: return "and";
        case IR_OR: return "or";
        case IR_XOR: return "xor";
        case IR_SHL: return "shl";
        case IR_SHR: return "shr";
        case IR_SAR: return "sar";
        case IR_NOT: return "not";
        case IR_NEG: return "neg";
        case IR_EQ: return "eq";
        case IR_NE: return "ne";
        case IR_SLT: return "slt";
        case IR_SLE: return "sle";
        case IR_SGT: return "sgt";
        case IR_SGE: return "sge";
        case IR_ULT: return "ult";
        case IR_ULE: return "ule";
        case IR_UGT: return "ugt";
        case IR_UGE: return "uge";
        case IR_LABEL: return "label";
        case IR_JUMP: return "jump";
        case IR_BRANCH: return "branch";
        case IR_CALL: return "call";
        case IR_RETURN: return "return";
        case IR_CONST_INT: return "const_int";
        case IR_CONST_FLOAT: return "const_float";
        case IR_CONST_STRING: return "const_string";
        case IR_SEXT: return "sext";
        case IR_ZEXT: return "zext";
        case IR_TRUNC: return "trunc";
        case IR_SITOFP: return "sitofp";
        case IR_UITOFP: return "uitofp";
        case IR_FPTOSI: return "fptosi";
        case IR_FPTOUI: return "fptoui";
        case IR_BITCAST: return "bitcast";
        case IR_PHI: return "phi";
        default: return "unknown";
    }
}

cJSON *value_to_json(IRValue *val) {
    if (!val) return cJSON_CreateNull();
    
    cJSON *obj = cJSON_CreateObject();
    
    switch (val->kind) {
        case IR_VALUE_TEMP: {
            char buf[32];
            snprintf(buf, sizeof(buf), "%%t%d", val->temp_id);
            cJSON_AddStringToObject(obj, "kind", "temp");
            cJSON_AddStringToObject(obj, "name", buf);
            break;
        }
        case IR_VALUE_GLOBAL:
            cJSON_AddStringToObject(obj, "kind", "global");
            cJSON_AddStringToObject(obj, "name", val->name ? val->name : "@unknown");
            break;
        case IR_VALUE_CONST_INT:
            cJSON_AddStringToObject(obj, "kind", "const_int");
            cJSON_AddNumberToObject(obj, "value", (double)val->int_val);
            break;
        case IR_VALUE_CONST_FLOAT:
            cJSON_AddStringToObject(obj, "kind", "const_float");
            cJSON_AddNumberToObject(obj, "value", val->float_val);
            break;
        case IR_VALUE_CONST_STRING:
            cJSON_AddStringToObject(obj, "kind", "const_string");
            cJSON_AddStringToObject(obj, "value", val->string_val ? val->string_val : "");
            break;
        case IR_VALUE_LABEL: {
            char buf[32];
            snprintf(buf, sizeof(buf), ".L%d", val->label_id);
            cJSON_AddStringToObject(obj, "kind", "label");
            cJSON_AddStringToObject(obj, "name", buf);
            break;
        }
        case IR_VALUE_UNDEF:
            cJSON_AddStringToObject(obj, "kind", "undef");
            break;
    }
    
    return obj;
}

cJSON *instr_to_json(IRInstr *instr) {
    if (!instr) return cJSON_CreateNull();
    
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "op", op_to_string(instr->op));
    
    if (instr->dest) {
        cJSON_AddItemToObject(obj, "dest", value_to_json(instr->dest));
    }
    
    switch (instr->op) {
        case IR_CALL:
            if (instr->call.callee) {
                cJSON_AddItemToObject(obj, "callee", value_to_json(instr->call.callee));
            }
            if (instr->call.arg_count > 0) {
                cJSON *args = cJSON_CreateArray();
                for (size_t i = 0; i < instr->call.arg_count; i++) {
                    cJSON_AddItemToArray(args, value_to_json(instr->call.args[i]));
                }
                cJSON_AddItemToObject(obj, "args", args);
            }
            break;
            
        case IR_BRANCH:
            if (instr->branch.cond) {
                cJSON_AddItemToObject(obj, "cond", value_to_json(instr->branch.cond));
            }
            if (instr->branch.true_block) {
                cJSON_AddNumberToObject(obj, "true_block", instr->branch.true_block->id);
            }
            if (instr->branch.false_block) {
                cJSON_AddNumberToObject(obj, "false_block", instr->branch.false_block->id);
            }
            break;
            
        case IR_JUMP:
            if (instr->jump.target) {
                cJSON_AddNumberToObject(obj, "target", instr->jump.target->id);
            }
            break;
            
        case IR_RETURN:
            if (instr->ret.ret_value) {
                cJSON_AddItemToObject(obj, "value", value_to_json(instr->ret.ret_value));
            }
            break;
            
        case IR_ALLOCA:
            cJSON_AddNumberToObject(obj, "size", (double)instr->alloca.size);
            cJSON_AddNumberToObject(obj, "align", (double)instr->alloca.align);
            break;
            
        case IR_PHI:
            if (instr->phi.incoming_count > 0) {
                cJSON *incoming = cJSON_CreateArray();
                for (size_t i = 0; i < instr->phi.incoming_count; i++) {
                    cJSON *pair = cJSON_CreateObject();
                    cJSON_AddItemToObject(pair, "value", 
                        value_to_json(instr->phi.incoming[i].value));
                    if (instr->phi.incoming[i].block) {
                        cJSON_AddNumberToObject(pair, "block", 
                            instr->phi.incoming[i].block->id);
                    }
                    cJSON_AddItemToArray(incoming, pair);
                }
                cJSON_AddItemToObject(obj, "incoming", incoming);
            }
            break;
            
        default:
            /* Most ops use arg1/arg2 */
            if (instr->arg1) {
                cJSON_AddItemToObject(obj, "arg1", value_to_json(instr->arg1));
            }
            if (instr->arg2) {
                cJSON_AddItemToObject(obj, "arg2", value_to_json(instr->arg2));
            }
            break;
    }
    
    return obj;
}

cJSON *block_to_json(IRBasicBlock *block) {
    if (!block) return cJSON_CreateNull();
    
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "id", block->id);
    
    if (block->name) {
        cJSON_AddStringToObject(obj, "name", block->name);
    }
    
    /* Emit instructions */
    cJSON *instrs = cJSON_CreateArray();
    for (IRInstr *instr = block->first; instr; instr = instr->next) {
        cJSON_AddItemToArray(instrs, instr_to_json(instr));
    }
    cJSON_AddItemToObject(obj, "instructions", instrs);
    
    return obj;
}

cJSON *function_to_json(IRFunction *func) {
    if (!func) return cJSON_CreateNull();
    
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "name", func->name ? func->name : "unknown");
    
    /* Emit parameters */
    if (func->param_count > 0) {
        cJSON *params = cJSON_CreateArray();
        for (size_t i = 0; i < func->param_count; i++) {
            cJSON_AddItemToArray(params, value_to_json(func->params[i]));
        }
        cJSON_AddItemToObject(obj, "params", params);
    }
    
    /* Emit blocks */
    cJSON *blocks = cJSON_CreateArray();
    for (IRBasicBlock *block = func->blocks; block; block = block->next) {
        cJSON_AddItemToArray(blocks, block_to_json(block));
    }
    cJSON_AddItemToObject(obj, "blocks", blocks);
    
    return obj;
}

void ir_emit_json(IRModule *module, const char *filename) {
    if (!module) return;
    
    cJSON *root = cJSON_CreateObject();
    
    if (module->source_file) {
        cJSON_AddStringToObject(root, "source", module->source_file);
    }
    
    /* Emit globals */
    if (module->global_count > 0) {
        cJSON *globals = cJSON_CreateArray();
        for (size_t i = 0; i < module->global_count; i++) {
            cJSON_AddItemToArray(globals, value_to_json(module->globals[i]));
        }
        cJSON_AddItemToObject(root, "globals", globals);
    }
    
    /* Emit functions */
    cJSON *funcs = cJSON_CreateArray();
    for (IRFunction *func = module->functions; func; func = func->next) {
        cJSON_AddItemToArray(funcs, function_to_json(func));
    }
    cJSON_AddItemToObject(root, "functions", funcs);
    
    /* Write to file */
    char *json_str = cJSON_Print(root);
    FILE *f = fopen(filename, "w");
    if (f) {
        fprintf(f, "%s\n", json_str);
        fclose(f);
    }
    
    free(json_str);
    cJSON_Delete(root);
}

/* --- Text IR Emission --- */

void ir_emit_text(IRModule *module, const char *filename) {
    if (!module) return;
    
    FILE *f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Failed to open %s for writing\n", filename);
        return;
    }
    
    /* Print module header */
    fprintf(f, "; ModuleID = '%s'\n\n", module->source_file ? module->source_file : "unknown");
    
    /* Print global variables */
    if (module->global_count > 0) {
        fprintf(f, "; Global variables\n");
        for (size_t i = 0; i < module->global_count; i++) {
            IRValue *global = module->globals[i];
            if (global->kind == IR_VALUE_CONST_STRING) {
                fprintf(f, "@%s = private constant [%zu x i8] c\"%s\\00\"\n", 
                        global->name, 
                        strlen(global->string_val) + 1,
                        global->string_val);
            }
        }
        fprintf(f, "\n");
    }
    
    /* Print functions */
    for (IRFunction *func = module->functions; func; func = func->next) {
        /* Function signature */
        fprintf(f, "define ");
        if (func->return_type && func->return_type->base && func->return_type->base->name) {
            fprintf(f, "%s ", func->return_type->base->name);
        } else {
            fprintf(f, "i32 ");
        }
        fprintf(f, "@%s(", func->name ? func->name : "unknown");
        
        /* Parameters */
        for (size_t i = 0; i < func->param_count; i++) {
            if (i > 0) fprintf(f, ", ");
            IRValue *param = func->params[i];
            if (param->type && param->type->name) {
                fprintf(f, "%s ", param->type->name);
            } else {
                fprintf(f, "i32 ");
            }
            fprintf(f, "%%t%d", param->temp_id);
        }
        fprintf(f, ") {\n");
        
        /* Basic blocks */
        for (IRBasicBlock *block = func->blocks; block; block = block->next) {
            /* Block label */
            if (block->name) {
                fprintf(f, "%s:  ; .L%d\n", block->name, block->id);
            } else {
                fprintf(f, ".L%d:\n", block->id);
            }
            
            /* Instructions */
            for (IRInstr *instr = block->first; instr; instr = instr->next) {
                fprintf(f, "  ");
                
                /* Destination */
                if (instr->dest) {
                    if (instr->dest->kind == IR_VALUE_TEMP) {
                        fprintf(f, "%%t%d = ", instr->dest->temp_id);
                    }
                }
                
                /* Opcode */
                fprintf(f, "%s ", op_to_string(instr->op));
                
                /* Operands based on instruction type */
                switch (instr->op) {
                    case IR_ALLOCA:
                        fprintf(f, "%zu, align %zu", 
                               instr->alloca.size, instr->alloca.align);
                        break;
                        
                    case IR_BRANCH:
                        print_value(f, instr->branch.cond);
                        fprintf(f, ", .L%d, .L%d", 
                               instr->branch.true_block ? instr->branch.true_block->id : 0,
                               instr->branch.false_block ? instr->branch.false_block->id : 0);
                        break;
                        
                    case IR_JUMP:
                        fprintf(f, ".L%d", instr->jump.target ? instr->jump.target->id : 0);
                        break;
                        
                    case IR_RETURN:
                        if (instr->ret.ret_value) {
                            print_value(f, instr->ret.ret_value);
                        } else {
                            fprintf(f, "void");
                        }
                        break;
                        
                    case IR_CALL:
                        fprintf(f, "@%s(", 
                               instr->call.callee && instr->call.callee->name ? instr->call.callee->name : "unknown");
                        for (size_t i = 0; i < instr->call.arg_count; i++) {
                            if (i > 0) fprintf(f, ", ");
                            print_value(f, instr->call.args[i]);
                        }
                        fprintf(f, ")");
                        break;
                        
                    case IR_PHI:
                        fprintf(f, "[");
                        for (size_t i = 0; i < instr->phi.incoming_count; i++) {
                            if (i > 0) fprintf(f, ", ");
                            print_value(f, instr->phi.incoming[i].value);
                            fprintf(f, ", .L%d", instr->phi.incoming[i].block ? instr->phi.incoming[i].block->id : 0);
                        }
                        fprintf(f, "]");
                        break;
                        
                    default:
                        if (instr->arg1) {
                            print_value(f, instr->arg1);
                        }
                        if (instr->arg2) {
                            fprintf(f, ", ");
                            print_value(f, instr->arg2);
                        }
                        break;
                }
                
                fprintf(f, "\n");
            }
            fprintf(f, "\n");
        }
        
        fprintf(f, "}\n\n");
    }
    
    fclose(f);
}

void print_value(FILE *f, IRValue *val) {
    if (!val || (uintptr_t)val < 0x1000) {
        fprintf(f, "null");
        return;
    }
    
    switch (val->kind) {
        case IR_VALUE_TEMP:
            fprintf(f, "%%t%d", val->temp_id);
            break;
        case IR_VALUE_GLOBAL:
            fprintf(f, "@%s", val->name ? val->name : "unknown");
            break;
        case IR_VALUE_CONST_INT:
            fprintf(f, "%lld", val->int_val);
            break;
        case IR_VALUE_CONST_FLOAT:
            fprintf(f, "%f", val->float_val);
            break;
        case IR_VALUE_CONST_STRING:
            fprintf(f, "@%s", val->name);
            break;
        case IR_VALUE_LABEL:
            fprintf(f, ".L%d", val->label_id);
            break;
        case IR_VALUE_UNDEF:
            fprintf(f, "undef");
            break;
        default:
            fprintf(f, "invalid_value");
            break;
    }
}

IRModule *ir_create_module(const char *source_file) {
	IRModule *mod = xcalloc(1, sizeof(IRModule));

	if (source_file) {
		mod->source_file = strdup(source_file);
	} else {
		mod->source_file = NULL;
	}

	mod->functions = NULL;
    mod->func_count = 0;
    mod->globals = NULL;
    mod->global_count = 0;
    mod->constants = NULL;
    mod->constant_count = 0;
    
    return mod;
}

IRFunction *ir_create_function(IRModule *mod, const char *name, ResolvedType *return_type) {
	IRFunction *func = xcalloc(1, sizeof(IRFunction));

	func->name = strdup(name);
    func->return_type = return_type;
    func->params = NULL;
    func->param_count = 0;
    func->entry = NULL;
    func->blocks = NULL;
    func->block_count = 0;
    func->temp_counter = 0;
    func->label_counter = 0;
    func->values = NULL;
    func->value_count = 0;
    func->next = NULL;
    func->var_map = NULL;
    func->var_map_count = 0;

	if (!mod->functions) {
        mod->functions = func;
    } else {
        IRFunction *last = mod->functions;
        while (last->next) last = last->next;
        last->next = func;
    }
    mod->func_count++;
    
    return func;
}

IRBasicBlock *ir_create_block(IRFunction *func, const char *name) {
	IRBasicBlock *block = xcalloc(1, sizeof(IRBasicBlock));

	block->id = func->label_counter++;
    block->name = name ? strdup(name) : NULL;
    block->first = NULL;
    block->last = NULL;
    block->preds = NULL;
    block->pred_count = 0;
    block->succs = NULL;
    block->succ_count = 0;
    block->next = NULL;

	if (!func->blocks) {
		func->blocks = block;
		func->entry = block;
	} else {
		IRBasicBlock *last = func->blocks;
        while (last->next) last = last->next;
        last->next = block;
	}
	func->block_count++;

	return block;
}

IRValue *ir_create_temp(IRFunction *func, ResolvedType *type) {
	IRValue *val = xcalloc(1, sizeof(IRValue));
    
    val->kind = IR_VALUE_TEMP;
    val->type = type;
    val->temp_id = func->temp_counter++;
    func->values = xrealloc(func->values, sizeof(IRValue *) * (func->value_count + 1));
    func->values[func->value_count++] = val;
    return val;
}

IRValue *ir_create_const_int(long long value, ResolvedType *type) {
	IRValue *val = xcalloc(1, sizeof(IRValue));

	val->kind = IR_VALUE_CONST_INT;
	val->type = type;
	val->int_val = value;

	return val;
}

IRValue *ir_create_const_float(double value, ResolvedType *type) {
	IRValue *val = xcalloc(1, sizeof(IRValue));

	val->kind = IR_VALUE_CONST_FLOAT;
	val->type = type;
	val->float_val = value;

	return val;
}

IRValue *ir_create_const_string(const char *value) {
	IRValue *val = xcalloc(1, sizeof(IRValue));

	val->kind = IR_VALUE_CONST_STRING;
	val->type = NULL;
	val->string_val = strdup(value);

	return val;
}

IRValue *ir_create_global(const char *name, ResolvedType *type) {
	IRValue *val = xcalloc(1, sizeof(IRValue));

	val->kind = IR_VALUE_GLOBAL;
	val->type = type;
	val->name = strdup(name);

	return val;
}

IRValue *ir_create_label(IRFunction *func) {
	IRValue *val = xcalloc(1, sizeof(IRValue));
    
    val->kind = IR_VALUE_LABEL;
    val->type = NULL;
    val->label_id = func->label_counter++;
    
    func->values = xrealloc(func->values, sizeof(IRValue *) * (func->value_count + 1));
    func->values[func->value_count++] = val;
    
    return val;
}

IRInstr *ir_emit(IRBasicBlock *block, IROp op, IRValue *dest) {
    IRInstr *instr = xcalloc(1, sizeof(IRInstr));
    
    instr->op = op;
    instr->dest = dest;
    instr->arg1 = NULL;
    instr->arg2 = NULL;
    instr->next = NULL;
    
    /* Append to block */
    if (!block->first) {
        block->first = instr;
        block->last = instr;
    } else {
        block->last->next = instr;
        block->last = instr;
    }
    
    return instr;
}

void ir_set_args(IRInstr *instr, IRValue *arg1, IRValue *arg2) {
    instr->arg1 = arg1;
    instr->arg2 = arg2;
}

void ir_set_call(IRInstr *instr, IRValue *callee, IRValue **args, 
                 size_t arg_count) {
    instr->call.callee = callee;
    instr->call.args = args;
    instr->call.arg_count = arg_count;
}

void ir_set_branch(IRInstr *instr, IRValue *cond, IRBasicBlock *true_block, IRBasicBlock *false_block) {
    instr->branch.cond = cond;
    instr->branch.true_block = true_block;
    instr->branch.false_block = false_block;
}

void ir_set_jump(IRInstr *instr, IRBasicBlock *target) {
    instr->jump.target = target;
}

void ir_set_return(IRInstr *instr, IRValue *ret_value) {
    instr->ret.ret_value = ret_value;
}

void lower_stmt(IRFunction *func, IRBasicBlock **current_block, Node *stmt, IRModule *mod, LoopContext *loop_ctx) {
    if (!stmt) return;

    switch (stmt->type) {
        case NODE_VAR_DECL: {
            size_t size = stmt->rtype ? stmt->rtype->size : 4;
            size_t align = stmt->rtype ? stmt->rtype->align : 4;

            IRValue *ptr = ir_create_temp(func, stmt->rtype);

            IRInstr *alloca_instr = ir_emit(*current_block, IR_ALLOCA, ptr);
            alloca_instr->alloca.size = size;
            alloca_instr->alloca.align = align;

            func->var_map = xrealloc(func->var_map, 
                sizeof(VarMapping) * (func->var_map_count + 1));
            func->var_map[func->var_map_count].name = strdup(stmt->var_decl.name);
            func->var_map[func->var_map_count].value = ptr;
            func->var_map_count++;

            if (stmt->var_decl.value) {
                IRValue *init_val = lower_expr(func, current_block, stmt->var_decl.value, mod, 0);
                IRInstr *store = ir_emit(*current_block, IR_STORE, NULL);
                ir_set_args(store, init_val, ptr);
            }
            break;
        }
        
        case NODE_RETURN: {
            /* Return statement - IMPLEMENTED */
            IRValue *ret_val = NULL;

            if (stmt->return_stmt.value) {
                ret_val = lower_expr(func, current_block, stmt->return_stmt.value, mod, 0);
            }

            IRInstr *ret_instr = ir_emit(*current_block, IR_RETURN, NULL);
            ir_set_return(ret_instr, ret_val);
            break;
        }
        
        case NODE_EXPR: {
            /* Expression statement - IMPLEMENTED */
            lower_expr(func, current_block, stmt->expr.expr, mod, 0);
            break;
        }
        
        case NODE_IF_STMT: {
            IRBasicBlock *then_block = ir_create_block(func, "if.then");
            IRBasicBlock *merge_block = ir_create_block(func, "if.merge");
            IRBasicBlock *else_block = NULL;

            IRValue *cond = lower_expr(func, current_block, stmt->if_stmt.if_cond, mod, 0);

            if (stmt->if_stmt.else_body || stmt->if_stmt.elif_count > 0) {
                else_block = ir_create_block(func, "if.else");
            } else {
                else_block = merge_block;
            }

            IRInstr *branch = ir_emit(*current_block, IR_BRANCH, NULL);
            ir_set_branch(branch, cond, then_block, else_block);

            *current_block = then_block;
            if (stmt->if_stmt.if_body) {
                for (size_t i = 0; stmt->if_stmt.if_body[i]; i++) {
                    lower_stmt(func, current_block, stmt->if_stmt.if_body[i], mod, loop_ctx);
                }
            }

            if (!(*current_block)->last || (*current_block)->last->op != IR_RETURN) {
                IRInstr *jmp = ir_emit(*current_block, IR_JUMP, NULL);
                ir_set_jump(jmp, merge_block);
            }

            if (stmt->if_stmt.elif_count > 0 || stmt->if_stmt.else_body) {
                *current_block = else_block;

                for (size_t i = 0; i < stmt->if_stmt.elif_count; i++) {
                    IRBasicBlock *elif_then = ir_create_block(func, "elif.then");
                    IRBasicBlock *elif_else = NULL;

                    if (i + 1 < stmt->if_stmt.elif_count) {
                        elif_else = ir_create_block(func, "elif.else");
                    } else if (stmt->if_stmt.else_body) {
                        elif_else = ir_create_block(func, "else");
                    } else {
                        elif_else = merge_block;
                    }

                    IRValue *elif_cond = lower_expr(func, current_block, stmt->if_stmt.elif_conds[i], mod, 0);

                    IRInstr *elif_branch = ir_emit(*current_block, IR_BRANCH, NULL);
                    ir_set_branch(elif_branch, elif_cond, elif_then, elif_else);

                    *current_block = elif_then;
                    if (stmt->if_stmt.elif_bodies[i]) {
                        for (size_t j = 0; stmt->if_stmt.elif_bodies[i][j]; j++) {
                            lower_stmt(func, current_block, stmt->if_stmt.elif_bodies[i][j], mod, loop_ctx);
                        }
                    }

                    if (!(*current_block)->last || (*current_block)->last->op != IR_RETURN) {
                        IRInstr *jmp = ir_emit(*current_block, IR_JUMP, NULL);
                        ir_set_jump(jmp, merge_block);
                    }
                    
                    *current_block = elif_else;
                }

                if (stmt->if_stmt.else_body) {
                    for (size_t i = 0; stmt->if_stmt.else_body[i]; i++) {
                        lower_stmt(func, current_block, stmt->if_stmt.else_body[i], mod, loop_ctx);
                    }
                    
                    if (!(*current_block)->last || (*current_block)->last->op != IR_RETURN) {
                        IRInstr *jmp = ir_emit(*current_block, IR_JUMP, NULL);
                        ir_set_jump(jmp, merge_block);
                    }
                }
            }
            *current_block = merge_block;
            break;
        }
        
        case NODE_WHILE_STMT: {
            IRBasicBlock *cond_block = ir_create_block(func, "while.cond");
            IRBasicBlock *body_block = ir_create_block(func, "while.body");
            IRBasicBlock *exit_block = ir_create_block(func, "while.exit");

            IRInstr *jmp_to_cond = ir_emit(*current_block, IR_JUMP, NULL);
            ir_set_jump(jmp_to_cond, cond_block);

            *current_block = cond_block;
            IRValue *cond = lower_expr(func, current_block, stmt->while_stmt.cond, mod, 0);

            IRInstr *branch = ir_emit(*current_block, IR_BRANCH, NULL);
            ir_set_branch(branch, cond, body_block, exit_block);


            LoopContext ctx = {
                .continue_target = cond_block,
                .break_target = exit_block,
                .parent = loop_ctx
            };
            *current_block = body_block;
            if (stmt->while_stmt.body) {
                for (size_t i = 0; stmt->while_stmt.body[i]; i++) {
                    lower_stmt(func, current_block, stmt->while_stmt.body[i], mod, &ctx);
                }
            }

            if (!(*current_block)->last || 
                ((*current_block)->last->op != IR_RETURN && 
                (*current_block)->last->op != IR_JUMP)) {
                IRInstr *jmp_back = ir_emit(*current_block, IR_JUMP, NULL);
                ir_set_jump(jmp_back, cond_block);
            }

            *current_block = exit_block;
            break;
        }
        
        case NODE_DO_WHILE_STMT: {
            IRBasicBlock *body_block = ir_create_block(func, "do-while.body");
            IRBasicBlock *cond_block = ir_create_block(func, "do-while.cond");
            IRBasicBlock *exit_block = ir_create_block(func, "do-while.exit");
            IRInstr *jump_to_body = ir_emit(*current_block, IR_JUMP, NULL);
            ir_set_jump(jump_to_body, body_block);

            LoopContext ctx = {
                .continue_target = cond_block,
                .break_target = exit_block,
                .parent = loop_ctx
            };


            *current_block = body_block;
            if (stmt->do_while_stmt.body) {
                for (size_t i = 0; stmt->do_while_stmt.body[i]; i++) {
                    lower_stmt(func, current_block, stmt->do_while_stmt.body[i], mod, &ctx);
                }
            }

            if (!(*current_block)->last || ((*current_block)->last->op != IR_RETURN && (*current_block)->last->op != IR_JUMP)) {
                IRInstr *jmp_to_cond = ir_emit(*current_block, IR_JUMP, NULL);
                ir_set_jump(jmp_to_cond, cond_block);
            }

            *current_block = cond_block;
            IRValue *cond = lower_expr(func, current_block, stmt->do_while_stmt.cond, mod, 0);

            IRInstr *branch = ir_emit(*current_block, IR_BRANCH, NULL);
            ir_set_branch(branch, cond, body_block, exit_block);
            
            *current_block = exit_block;
            break;
        }
        
        case NODE_FOR_STMT: {
            if (stmt->for_stmt.init) {
                lower_stmt(func, current_block, stmt->for_stmt.init, mod, loop_ctx);
            }

            IRBasicBlock *cond_block = ir_create_block(func, "for.cond");
            IRBasicBlock *body_block = ir_create_block(func, "for.body");
            IRBasicBlock *inc_block = ir_create_block(func, "for.inc");
            IRBasicBlock *exit_block = ir_create_block(func, "for.exit");

            IRInstr *jump_to_cond = ir_emit(*current_block, IR_JUMP, NULL);
            ir_set_jump(jump_to_cond, cond_block);

            *current_block = cond_block;
            if (stmt->for_stmt.cond) {
                IRValue *cond = lower_expr(func, current_block,  stmt->for_stmt.cond->expr.expr, mod, 0);
                IRInstr *branch = ir_emit(*current_block, IR_BRANCH, NULL);
                ir_set_branch(branch, cond, body_block, exit_block);
            } else {
                IRInstr *jmp = ir_emit(*current_block, IR_JUMP, NULL);
                ir_set_jump(jmp, body_block);
            }

            LoopContext ctx = {
                .continue_target = inc_block,
                .break_target = exit_block,
                .parent = loop_ctx
            };

            *current_block = body_block;
            if (stmt->for_stmt.body) {
                for (size_t i = 0; stmt->for_stmt.body[i]; i++) {
                    lower_stmt(func, current_block, stmt->for_stmt.body[i], mod, &ctx);
                }
            }

            if (!(*current_block)->last || ((*current_block)->last->op != IR_RETURN && (*current_block)->last->op != IR_JUMP)) {
                IRInstr *jmp_to_inc = ir_emit(*current_block, IR_JUMP, NULL);
                ir_set_jump(jmp_to_inc, inc_block);
            }

            *current_block = inc_block;
            if (stmt->for_stmt.inc) {
                lower_expr(func, current_block, stmt->for_stmt.inc, mod, 0);
            }

            IRInstr *jmp_back = ir_emit(*current_block, IR_JUMP, NULL);
            ir_set_jump(jmp_back, cond_block);
            *current_block = exit_block;
            break;
        }
        
        case NODE_SWITCH_STMT: {
            IRValue *switch_val = lower_expr(func, current_block, stmt->switch_stmt.expression, mod, 0);

            IRBasicBlock *exit_block = ir_create_block(func, "switch.exit");
            IRBasicBlock *default_block = NULL;
            IRBasicBlock **case_blocks = xmalloc(sizeof(IRBasicBlock *) * stmt->switch_stmt.case_count);

            for (size_t i = 0; i < stmt->switch_stmt.case_count; i++) {
                char buf[128];
                if (stmt->switch_stmt.cases[i]->kind == EXPR_LITERAL) {
                    sprintf(buf, "switch.case_%s", stmt->switch_stmt.cases[i]->literal->raw);
                } else {
                    sprintf(buf, "switch.case.%zu", i + 1);
                }
                case_blocks[i] = ir_create_block(func, buf);
            }

            if (stmt->switch_stmt.default_body) {
                default_block = ir_create_block(func, "switch.default");
            } else {
                default_block = exit_block;
            }

            IRBasicBlock *next_check = *current_block;
            for (size_t i = 0; i < stmt->switch_stmt.case_count; i++) {
                *current_block = next_check;
                IRValue *case_val = lower_expr(func, current_block, stmt->switch_stmt.cases[i], mod, 0);
                
                IRValue *cmp = ir_create_temp(func, NULL);
                IRInstr *cmp_instr = ir_emit(*current_block, IR_EQ, cmp);
                ir_set_args(cmp_instr, switch_val, case_val);
                
                if (i + 1 < stmt->switch_stmt.case_count) {
                    next_check = ir_create_block(func, "switch.check");
                } else {
                    next_check = default_block;  // Last case → go to default
                }
                
                IRInstr *branch = ir_emit(*current_block, IR_BRANCH, NULL);
                ir_set_branch(branch, cmp, case_blocks[i], next_check);
            }
            
            for (size_t i = 0; i < stmt->switch_stmt.case_count; i++) {
                *current_block = case_blocks[i];
                
                if (stmt->switch_stmt.case_bodies[i]) {
                    for (size_t j = 0; stmt->switch_stmt.case_bodies[i][j]; j++) {
                        lower_stmt(func, current_block, stmt->switch_stmt.case_bodies[i][j], mod, loop_ctx);
                    }
                }
                
                if (!(*current_block)->last || (*current_block)->last->op != IR_RETURN) {
                    IRInstr *jmp = ir_emit(*current_block, IR_JUMP, NULL);
                    ir_set_jump(jmp, exit_block);
                }
            }
            
            if (stmt->switch_stmt.default_body) {
                *current_block = default_block;
                
                for (size_t i = 0; stmt->switch_stmt.default_body[i]; i++) {
                    lower_stmt(func, current_block, 
                            stmt->switch_stmt.default_body[i], mod, loop_ctx);
                }
                
                if (!(*current_block)->last || (*current_block)->last->op != IR_RETURN) {
                    IRInstr *jmp = ir_emit(*current_block, IR_JUMP, NULL);
                    ir_set_jump(jmp, exit_block);
                }
            }

            *current_block = exit_block;

            xfree(case_blocks);
            break;
        }
        
        case NODE_MISC: {
            if (!stmt->misc.name) break;

            if (strcmp(stmt->misc.name, "break") == 0) {
                if (!loop_ctx) {
                    fprintf(stderr, "Error: 'break' outside of loop\n");
                    break;
                }
                IRInstr *jmp = ir_emit(*current_block, IR_JUMP, NULL);
                ir_set_jump(jmp, loop_ctx->break_target);
            } else if (strcmp(stmt->misc.name, "continue") == 0) {
                if (!loop_ctx) {
                    fprintf(stderr, "Error: 'continue' outside of loop\n");
                    break;
                }
                
                IRInstr *jmp = ir_emit(*current_block, IR_JUMP, NULL);
                ir_set_jump(jmp, loop_ctx->continue_target);
            }
            break;
        }
        
        case NODE_FUNC_DECL:
        case NODE_PROGRAM:
        case NODE_ENUM_DECL:
        case NODE_STRUCT_DECL:
        case NODE_UNION_DECL:
        case NODE_TYPE:
        case NODE_TYPEDEF:
            break;

        case NODE_ARRAY: {
            ResolvedType *arr_type = stmt->rtype;

            IRValue *arr_ptr = ir_create_temp(func, arr_type);
            IRInstr *alloca = ir_emit(*current_block, IR_ALLOCA, arr_ptr);
            alloca->alloca.size = arr_type->size;
            alloca->alloca.align = arr_type->align;

            func->var_map = xrealloc(func->var_map, sizeof(VarMapping) * (func->var_map_count + 1));
            func->var_map[func->var_map_count].name = strdup(stmt->array.name);
            func->var_map[func->var_map_count].value = arr_ptr;
            func->var_map_count++;

            if (stmt->array.value && stmt->array.value->kind == EXPR_SET) {
                Expr **flat_elements = NULL;
                size_t flat_count = 0;
                flatten_set(stmt->array.value, &flat_elements, &flat_count);


                size_t elem_size = arr_type->base->size;

                for (size_t i = 0; i < flat_count; i++) {
                    IRValue *offset = ir_create_const_int(i * elem_size, NULL);
                    mod->constants = xrealloc(mod->constants, sizeof(IRValue *) * (mod->constant_count + 1));
                    mod->constants[mod->constant_count++] = offset;

                    IRValue *elem_ptr = ir_create_temp(func, NULL);
                    IRInstr *add = ir_emit(*current_block, IR_ADD, elem_ptr);
                    ir_set_args(add, arr_ptr, offset);

                    IRValue *val = lower_expr(func, current_block, flat_elements[i], mod, 0);
                    IRInstr *store = ir_emit(*current_block, IR_STORE, NULL);
                    ir_set_args(store, val, elem_ptr);
                }
                xfree(flat_elements);
            }
            break;
        }

            
        default:
            fprintf(stderr, "Unhandled statement type: %d\n", stmt->type);
            break;
    }
}

IRValue *lower_expr(IRFunction *func, IRBasicBlock **block, Expr *expr, IRModule *mod, int is_lvalue) {
    if (!expr) return NULL;

    switch (expr->kind) {
        case EXPR_LITERAL: {
            IRValue *const_val = NULL;
        
            /* Use pre-parsed literal value from the Literal struct */
            switch (*expr->literal->kind) {
                case LIT_INT:
                    const_val = ir_create_const_int(expr->literal->int_val, NULL);
                    mod->constants = xrealloc(mod->constants, sizeof(IRValue *) * (mod->constant_count + 1));
                    mod->constants[mod->constant_count++] = const_val;
                    break;
                    
                case LIT_FLOAT:
                    const_val = ir_create_const_float(expr->literal->float_val, NULL);
                    mod->constants = xrealloc(mod->constants, sizeof(IRValue *) * (mod->constant_count + 1));
                    mod->constants[mod->constant_count++] = const_val;
                    break;
                    
                case LIT_CHAR:
                    /* Treat char as small integer */
                    const_val = ir_create_const_int((long long)expr->literal->char_val, NULL);
                    mod->constants = xrealloc(mod->constants, sizeof(IRValue *) * (mod->constant_count + 1));
                    mod->constants[mod->constant_count++] = const_val;
                    break;
                    
                case LIT_BOOL:
                    const_val = ir_create_const_int((long long)expr->literal->bool_val, NULL);
                    mod->constants = xrealloc(mod->constants, sizeof(IRValue *) * (mod->constant_count + 1));
                    mod->constants[mod->constant_count++] = const_val;
                    break;
                    
                case LIT_STRING: {
                    int string_counter = 0;
                    char name[32];
                    snprintf(name, sizeof(name), ".str.%d", string_counter++);
                    
                    IRValue *global_str = ir_create_global(name, NULL);
                    global_str->kind = IR_VALUE_CONST_STRING;
                    global_str->string_val = strdup(expr->literal->string_val);
                    
                    /* Add to module's globals (not constants!) */
                    mod->globals = xrealloc(mod->globals, 
                        sizeof(IRValue *) * (mod->global_count + 1));
                    mod->globals[mod->global_count++] = global_str;
                    
                    /* Return pointer to global string */
                    return global_str;
                }
            }
            
            return const_val;
        }
        
        case EXPR_BINARY: {
            if (strcmp(expr->binary.op, "=") == 0) {
                IRValue *rhs = lower_expr(func, block, expr->binary.right, mod, 0);
                IRValue *lhs_ptr = lower_expr(func, block, expr->binary.left, mod, 1);
                IRInstr *store = ir_emit(*block, IR_STORE, NULL);
                ir_set_args(store, rhs, lhs_ptr);
                return rhs;
            }
        
            if (strcmp(expr->binary.op, "&&") == 0) {
                IRBasicBlock *orig_block = *block;
                IRBasicBlock *eval_right = ir_create_block(func, "land.rhs");
                IRBasicBlock *end_block = ir_create_block(func, "land.end");
        
                IRValue *left = lower_expr(func, block, expr->binary.left, mod, 0);
                IRValue *result = ir_create_temp(func, NULL);
        
                IRInstr *branch = ir_emit(*block, IR_BRANCH, NULL);
                ir_set_branch(branch, left, eval_right, end_block);
        
                *block = end_block;
                IRValue *right = lower_expr(func, &eval_right, expr->binary.right, mod, 0);
        
                IRInstr *jmp = ir_emit(eval_right, IR_JUMP, NULL);
                ir_set_jump(jmp, end_block);
        
                IRInstr *phi = ir_emit(end_block, IR_PHI, result);
                phi->phi.incoming = xcalloc(2, sizeof(IRPhiIncoming));
                phi->phi.incoming_count = 2;
        
                IRValue *zero = ir_create_const_int(0, NULL);
                mod->constants = xrealloc(mod->constants, sizeof(IRValue *) * (mod->constant_count + 1));
                mod->constants[mod->constant_count++] = zero;
                phi->phi.incoming[0].value = zero;
                phi->phi.incoming[0].block = orig_block;
                phi->phi.incoming[1].value = right;
                phi->phi.incoming[1].block = eval_right;
                
                return result;
            }
            
            if (strcmp(expr->binary.op, "||") == 0) {
                IRBasicBlock *orig_block = *block;
                IRBasicBlock *eval_right = ir_create_block(func, "lor.rhs");
                IRBasicBlock *end_block = ir_create_block(func, "lor.end");
                
                IRValue *left = lower_expr(func, block, expr->binary.left, mod, 0);
                IRValue *result = ir_create_temp(func, NULL);
                
                IRInstr *branch = ir_emit(*block, IR_BRANCH, NULL);
                ir_set_branch(branch, left, end_block, eval_right);
                
                *block = end_block;
                IRValue *right = lower_expr(func, &eval_right, expr->binary.right, mod, 0);
                IRInstr *jmp = ir_emit(eval_right, IR_JUMP, NULL);
                ir_set_jump(jmp, end_block);
        
                IRInstr *phi = ir_emit(end_block, IR_PHI, result);
                phi->phi.incoming = xcalloc(2, sizeof(IRPhiIncoming));
                phi->phi.incoming_count = 2;
        
                IRValue *one = ir_create_const_int(1, NULL);
                mod->constants = xrealloc(mod->constants, sizeof(IRValue *) * (mod->constant_count + 1));
                mod->constants[mod->constant_count++] = one;
                phi->phi.incoming[0].value = one;
                phi->phi.incoming[0].block = orig_block;
                phi->phi.incoming[1].value = right;
                phi->phi.incoming[1].block = eval_right;
                
                return result;
            }
        
            if (strcmp(expr->binary.op, "+=") == 0 || strcmp(expr->binary.op, "-=") == 0 || 
                strcmp(expr->binary.op, "*=") == 0 || strcmp(expr->binary.op, "/=") == 0 || 
                strcmp(expr->binary.op, "%=") == 0 || strcmp(expr->binary.op, "&=") == 0 || 
                strcmp(expr->binary.op, "|=") == 0 || strcmp(expr->binary.op, "^=") == 0 || 
                strcmp(expr->binary.op, "<<=") == 0 || strcmp(expr->binary.op, ">>=") == 0) {
                
                IRValue *lhs_ptr = lower_expr(func, block, expr->binary.left, mod, 1);
                IRValue *lhs_val = ir_create_temp(func, NULL);
                IRInstr *load = ir_emit(*block, IR_LOAD, lhs_val);
                ir_set_args(load, lhs_ptr, NULL);
        
                IRValue *rhs = lower_expr(func, block, expr->binary.right, mod, 0);
        
                IROp op;
                if (strcmp(expr->binary.op, "+=") == 0) op = IR_ADD;
                else if (strcmp(expr->binary.op, "-=") == 0) op = IR_SUB;
                else if (strcmp(expr->binary.op, "*=") == 0) op = IR_MUL;
                else if (strcmp(expr->binary.op, "/=") == 0) op = IR_SDIV;
                else if (strcmp(expr->binary.op, "%=") == 0) op = IR_SMOD;
                else if (strcmp(expr->binary.op, "&=") == 0) op = IR_AND;
                else if (strcmp(expr->binary.op, "|=") == 0) op = IR_OR;
                else if (strcmp(expr->binary.op, "^=") == 0) op = IR_XOR;
                else if (strcmp(expr->binary.op, "<<=") == 0) op = IR_SHL;
                else if (strcmp(expr->binary.op, ">>=") == 0) op = IR_SHR;
                else return NULL;
        
                IRValue *result = ir_create_temp(func, NULL);
                IRInstr *arith = ir_emit(*block, op, result);
                ir_set_args(arith, lhs_val, rhs);
        
                IRInstr *store = ir_emit(*block, IR_STORE, NULL);
                ir_set_args(store, result, lhs_ptr);
                
                return result;
            }
        
            /* Check for pointer arithmetic */
            ResolvedType *left_type = expr->binary.left->inferred_type;
            ResolvedType *right_type = expr->binary.right->inferred_type;
        
            /* Case 1: ptr + int or ptr - int */
            if ((strcmp(expr->binary.op, "+") == 0 || strcmp(expr->binary.op, "-") == 0) &&
                left_type && left_type->kind == RT_POINTER && 
                right_type && right_type->kind == RT_BUILTIN) {
                
                IRValue *ptr = lower_expr(func, block, expr->binary.left, mod, 0);
                IRValue *offset = lower_expr(func, block, expr->binary.right, mod, 0);
                
                /* Scale offset by element size */
                size_t elem_size = left_type->base->size;
                
                if (elem_size != 1) {
                    IRValue *scale = ir_create_const_int(elem_size, NULL);
                    mod->constants = xrealloc(mod->constants, 
                        sizeof(IRValue *) * (mod->constant_count + 1));
                    mod->constants[mod->constant_count++] = scale;
                    
                    IRValue *scaled = ir_create_temp(func, NULL);
                    IRInstr *mul = ir_emit(*block, IR_MUL, scaled);
                    ir_set_args(mul, offset, scale);
                    offset = scaled;
                }
                
                IRValue *result = ir_create_temp(func, NULL);
                IROp op = (strcmp(expr->binary.op, "+") == 0) ? IR_ADD : IR_SUB;
                IRInstr *instr = ir_emit(*block, op, result);
                ir_set_args(instr, ptr, offset);
                
                return result;
            }
        
            /* Case 2: int + ptr */
            if (strcmp(expr->binary.op, "+") == 0 &&
                left_type && left_type->kind == RT_BUILTIN &&
                right_type && right_type->kind == RT_POINTER) {
                
                IRValue *offset = lower_expr(func, block, expr->binary.left, mod, 0);
                IRValue *ptr = lower_expr(func, block, expr->binary.right, mod, 0);
                
                /* Scale offset by element size */
                size_t elem_size = right_type->base->size;
                
                if (elem_size != 1) {
                    IRValue *scale = ir_create_const_int(elem_size, NULL);
                    mod->constants = xrealloc(mod->constants, 
                        sizeof(IRValue *) * (mod->constant_count + 1));
                    mod->constants[mod->constant_count++] = scale;
                    
                    IRValue *scaled = ir_create_temp(func, NULL);
                    IRInstr *mul = ir_emit(*block, IR_MUL, scaled);
                    ir_set_args(mul, offset, scale);
                    offset = scaled;
                }
                
                IRValue *result = ir_create_temp(func, NULL);
                IRInstr *add = ir_emit(*block, IR_ADD, result);
                ir_set_args(add, ptr, offset);
                
                return result;
            }
        
            /* Regular arithmetic */
            IRValue *left = lower_expr(func, block, expr->binary.left, mod, 0);
            IRValue *right = lower_expr(func, block, expr->binary.right, mod, 0);
            
            IROp op;
            if (strcmp(expr->binary.op, "+") == 0) op = IR_ADD;
            else if (strcmp(expr->binary.op, "-") == 0) op = IR_SUB;
            else if (strcmp(expr->binary.op, "*") == 0) op = IR_MUL;
            else if (strcmp(expr->binary.op, "/") == 0) op = IR_SDIV;
            else if (strcmp(expr->binary.op, "%") == 0) op = IR_SMOD;
            else if (strcmp(expr->binary.op, ">>") == 0) op = IR_SHR;
            else if (strcmp(expr->binary.op, "<<") == 0) op = IR_SHL;
            else if (strcmp(expr->binary.op, "&") == 0) op = IR_AND;
            else if (strcmp(expr->binary.op, "|") == 0) op = IR_OR;
            else if (strcmp(expr->binary.op, "^") == 0) op = IR_XOR;
            else if (strcmp(expr->binary.op, ">") == 0) op = IR_SGT;
            else if (strcmp(expr->binary.op, "<") == 0) op = IR_SLT;
            else if (strcmp(expr->binary.op, ">=") == 0) op = IR_SGE;
            else if (strcmp(expr->binary.op, "<=") == 0) op = IR_SLE;
            else if (strcmp(expr->binary.op, "==") == 0) op = IR_EQ;
            else if (strcmp(expr->binary.op, "!=") == 0) op = IR_NE;
            else return NULL;
            
            IRValue *result = ir_create_temp(func, NULL);
            IRInstr *instr = ir_emit(*block, op, result);
            ir_set_args(instr, left, right);
            
            return result;
        }
        
        case EXPR_GROUPING:
            return lower_expr(func, block, expr->group.expr, mod, 0);
        
        case EXPR_CALL: {
            IRValue **args = NULL;
            if (expr->call.arg_count > 0) {
                args = xcalloc(expr->call.arg_count, sizeof(IRValue *));
                for (size_t i = 0; i < expr->call.arg_count; i++) {
                    args[i] = lower_expr(func, block, expr->call.args[i], mod, 0);
                }
            }
            
            IRValue *callee = ir_create_global(expr->call.func, NULL);
            mod->constants = xrealloc(mod->constants,
                sizeof(IRValue *) * (mod->constant_count + 1));
            mod->constants[mod->constant_count++] = callee;
            
            IRValue *result = ir_create_temp(func, NULL);
            IRInstr *instr = ir_emit(*block, IR_CALL, result);
            ir_set_call(instr, callee, args, expr->call.arg_count);
            
            return result;
        }
        
        case EXPR_IDENTIFIER: {
            /* Look up variable */
            for (size_t i = 0; i < func->var_map_count; i++) {
                if (strcmp(func->var_map[i].name, expr->identifier) == 0) {
                    IRValue *ptr = func->var_map[i].value;

                    if (is_lvalue) {
                        return ptr;
                    } else {
                        IRValue *result = ir_create_temp(func, NULL);
                        
                        IRInstr *load = ir_emit(*block, IR_LOAD, result);
                        ir_set_args(load, ptr, NULL);
                        
                        return result;
                    }
                }
            }
            
            return ir_create_global(expr->identifier, NULL);
        }
        
        case EXPR_UNARY: {
            if (strcmp(expr->unary.op, "++") == 0 || strcmp(expr->unary.op, "--") == 0) {
                if (expr->unary.operand->kind != EXPR_IDENTIFIER) {
                    fprintf(stderr, "Error: Operand of %s must be a variable\n", expr->unary.op);
                    return NULL;
                }
                
                IRValue *ptr = NULL;
                for (size_t i = 0; i < func->var_map_count; i++) {
                    if (strcmp(func->var_map[i].name, expr->unary.operand->identifier) == 0) {
                        ptr = func->var_map[i].value;
                        break;
                    }
                }
                
                if (!ptr) {
                    fprintf(stderr, "Error: Undefined variable '%s'\n", expr->unary.operand->identifier);
                    return NULL;
                }
                
                IRValue *old_val = ir_create_temp(func, NULL);
                IRInstr *load = ir_emit(*block, IR_LOAD, old_val);
                ir_set_args(load, ptr, NULL);
                
                /* Create constant 1 */
                IRValue *one = ir_create_const_int(1, NULL);
                mod->constants = xrealloc(mod->constants, sizeof(IRValue *) * (mod->constant_count + 1));
                mod->constants[mod->constant_count++] = one;
                
                IRValue *new_val = ir_create_temp(func, NULL);
                IROp op = (strcmp(expr->unary.op, "++") == 0) ? IR_ADD : IR_SUB;
                IRInstr *arith = ir_emit(*block, op, new_val);
                ir_set_args(arith, old_val, one);
                
                IRInstr *store = ir_emit(*block, IR_STORE, NULL);
                ir_set_args(store, new_val, ptr);
                
                if (expr->unary.order == 1) {
                    return new_val;
                } else {  /* postfix */
                    return old_val;
                }
            }

            if (strcmp(expr->unary.op, "&") == 0) {
                IRValue *addr = lower_expr(func, block, expr->unary.operand, mod, 1);
                return addr;
            }

            if (strcmp(expr->unary.op, "*") == 0) {
                IRValue *ptr = lower_expr(func, block, expr->unary.operand, mod, 0);
                if (!ptr) return NULL;

                if (is_lvalue) {
                    return ptr;
                } else {
                    IRValue *result = ir_create_temp(func, NULL);
                    IRInstr *load = ir_emit(*block, IR_LOAD, result);
                    ir_set_args(load, ptr, NULL);
                    return result;
                }
            }
            
            IRValue *operand = lower_expr(func, block, expr->unary.operand, mod, 0);
            if (!operand) return NULL;

            IROp op;

            if (strcmp(expr->unary.op, "-") == 0) {
                /* Unary minus: negate */
                op = IR_NEG;
            } else if (strcmp(expr->unary.op, "~") == 0) {
                /* Bitwise NOT */
                op = IR_NOT;
            } else if (strcmp(expr->unary.op, "!") == 0) {
                /* Logical NOT: compare with 0 */
                IRValue *zero = ir_create_const_int(0, NULL);
                mod->constants = xrealloc(mod->constants,
                    sizeof(IRValue *) * (mod->constant_count + 1));
                mod->constants[mod->constant_count++] = zero;
                
                IRValue *result = ir_create_temp(func, NULL);
                IRInstr *cmp = ir_emit(*block, IR_EQ, result);
                ir_set_args(cmp, operand, zero);
                return result;
            } else if (strcmp(expr->unary.op, "+") == 0) {
                return operand;
            } else if (strcmp(expr->unary.op, "*") == 0) {
                fprintf(stderr, "Error: Pointer dereference not yet implemented\n");
                return NULL;
            } else {
                fprintf(stderr, "Error: Unknown unary operator '%s'\n", expr->unary.op);
                return NULL;
            }

            IRValue *result = ir_create_temp(func, NULL);
            IRInstr *instr = ir_emit(*block, op, result);
            ir_set_args(instr, operand, NULL);
            return result;
        }

        case EXPR_TERNARY: {
            IRBasicBlock *true_block = ir_create_block(func, "ternary.true");
            IRBasicBlock *false_block = ir_create_block(func, "ternary.false");
            IRBasicBlock *merge_block = ir_create_block(func, "ternary.merge");

            IRValue *cond = lower_expr(func, block, expr->ternary.cond, mod, 0);

            IRInstr *branch = ir_emit(*block, IR_BRANCH, NULL);
            ir_set_branch(branch, cond, true_block, false_block);

            *block = true_block;
            IRValue *true_val = lower_expr(func, block, expr->ternary.true_expr, mod, 0);
            IRInstr *true_jmp = ir_emit(*block, IR_JUMP, NULL);
            ir_set_jump(true_jmp, merge_block);

            *block = false_block;
            IRValue *false_val = lower_expr(func, block, expr->ternary.false_expr, mod, 0);
            IRInstr *false_jmp = ir_emit(*block, IR_JUMP, NULL);
            ir_set_jump(false_jmp, merge_block);

            *block = merge_block;
            IRValue *result = ir_create_temp(func, NULL);
            IRInstr *phi = ir_emit(*block, IR_PHI, result);
            phi->phi.incoming = xcalloc(2, sizeof(IRPhiIncoming));
            phi->phi.incoming_count = 2;
            phi->phi.incoming[0].value = true_val;
            phi->phi.incoming[0].block = true_block;
            phi->phi.incoming[1].value = false_val;
            phi->phi.incoming[1].block = false_block;
            return result;
        }

        case EXPR_MEMBER: {
            IRValue *base_ptr;

            if (expr->member.is_arrow) {
                base_ptr = lower_expr(func, block, expr->member.object, mod, 0);
            } else {
                base_ptr = lower_expr(func, block, expr->member.object, mod, 1);
            }

            IRValue *member_ptr = base_ptr;
            if (expr->member.offset > 0) {
                IRValue *offset_val = ir_create_const_int(expr->member.offset, NULL);
                mod->constants = xrealloc(mod->constants, sizeof(IRValue *) * (mod->constant_count + 1));
                mod->constants[mod->constant_count++] = offset_val;

                member_ptr = ir_create_temp(func, NULL);
                IRInstr *add = ir_emit(*block, IR_ADD, member_ptr);
                ir_set_args(add, base_ptr, offset_val);
            }

            if (is_lvalue) {
                return member_ptr;
            } else {
                IRValue *result = ir_create_temp(func, NULL);
                IRInstr *load = ir_emit(*block, IR_LOAD, result);
                ir_set_args(load, member_ptr, NULL);
                return result;
            }
        }

        case EXPR_SIZEOF: {
            IRValue *size_const = ir_create_const_int(expr->sizeof_expr.computed_size, NULL);
            mod->constants = xrealloc(mod->constants, sizeof(IRValue *) * (mod->constant_count + 1));
            mod->constants[mod->constant_count++] = size_const;
            return size_const;
        }

        case EXPR_CAST: {
            /* Get source and target types */
            ResolvedType *source_type = expr->cast.expr->inferred_type;
            ResolvedType *target_type = expr->inferred_type;
            
            if (!source_type || !target_type) {
                fprintf(stderr, "Error: Missing type information for cast\n");
                return NULL;
            }
            
            IRValue *source = lower_expr(func, block, expr->cast.expr, mod, 0);
            
            IROp cast_op = determine_cast_op(source_type, target_type);

            if (cast_op == IR_NOP) {
                return source;
            }

            IRValue *result = ir_create_temp(func, NULL);
            IRInstr *cast = ir_emit(*block, cast_op, result);
            ir_set_args(cast, source, NULL);
            
            return result;
        }

        case EXPR_INDEX: {
            IRValue *arr_ptr = lower_expr(func, block, expr->index.array, mod, 1);

            IRValue *index = lower_expr(func, block, expr->index.index, mod, 0);

            ResolvedType *arr_type = expr->index.array->inferred_type;

            if (!arr_type || arr_type->kind != RT_ARRAY) {
                fprintf(stderr, "[!] Error: Cannot generate IR for non-array index\n");
                return NULL;
            }

            size_t elem_size = arr_type->base->size;

            IRValue *elem_size_val = ir_create_const_int(elem_size, NULL);
            mod->constants = xrealloc(mod->constants, sizeof(IRValue *) * (mod->constant_count + 1));
            mod->constants[mod->constant_count++] = elem_size_val;

            IRValue *byte_offset = ir_create_temp(func, NULL);
            IRInstr *mul = ir_emit(*block, IR_MUL, byte_offset);
            ir_set_args(mul, index, elem_size_val);

            IRValue *elem_ptr = ir_create_temp(func, NULL);
            IRInstr *add = ir_emit(*block, IR_ADD, elem_ptr);
            ir_set_args(add, arr_ptr, byte_offset);

            if (is_lvalue) {
                return elem_ptr;
            } else {
                IRValue *result = ir_create_temp(func, NULL);
                IRInstr *load = ir_emit(*block, IR_LOAD, result);
                ir_set_args(load, elem_ptr, NULL);
                return result;
            }
        }
        default:
            return NULL;
    }
}

IROp determine_cast_op(ResolvedType *source, ResolvedType *target) {
    if (source == target || 
        (source->kind == target->kind && source->size == target->size)) {
        return IR_NOP;
    }

    if (source->kind == RT_POINTER || target->kind == RT_POINTER) {
        return IR_BITCAST;
    }

    if (source->kind == RT_BUILTIN && target->kind == RT_BUILTIN) {
        int src_is_float = source->is_floating;
        int tgt_is_float = target->is_floating;

        if (!src_is_float && tgt_is_float) {
            return source->is_signed ? IR_SITOFP : IR_UITOFP;
        }

        if (src_is_float && !tgt_is_float) {
            return target->is_signed ? IR_FPTOSI : IR_FPTOUI;
        }

        if (!src_is_float && !tgt_is_float) {
            if (target->size > source->size) {
                return source->is_signed ? IR_SEXT : IR_ZEXT;
            } else if (target->size < source->size) {
                return IR_TRUNC;
            } else {
                return IR_BITCAST;
            }
        }
        return IR_BITCAST;
    }
    
    if (source->kind == RT_ENUM || target->kind == RT_ENUM) {
        return IR_BITCAST;
    }
    
    return IR_BITCAST;
}

void flatten_set(Expr *set, Expr ***out_elements, size_t *out_count) {
    if (set->kind != EXPR_SET) {
        *out_elements = xrealloc(*out_elements, sizeof(Expr *) * (*out_count + 1));
        (*out_elements)[(*out_count)++] = set;
        return;
    }

    for (size_t i = 0; i < set->set.element_count; i++) {
        flatten_set(set->set.elements[i], out_elements, out_count);
    }
}