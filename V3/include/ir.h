/*
	* include/ir.h - [Enter description]
	* Author:   Amity
	* Date:     Wed Feb  4 01:04:19 2026
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

#ifndef IR_H
#define IR_H
#include "SA.h"
/* --- Typedefs - Structs - Enums ---*/

/* IR Opcodes */
typedef enum {
    IR_NOP,
	/* Memory */
	IR_ALLOCA,                         // %t1 = alloca <type>
	IR_LOAD,                           // %t1 = load %ptr
	IR_STORE,                          // stores %value, %ptr

	/* Arithmetic */
	IR_ADD,                            // %t1 = add %a, %b
	IR_SUB,                            // %t1 = sub %a, %b
	IR_MUL,                            // %t1 = mul %a, %b
	IR_SDIV,                           // %t1 = sdiv %a, %b (signed)
	IR_UDIV,                           // %t1 = udiv %a, %b (unsigned)
	IR_SMOD,                           // %t1 = smod %a, %b (signed)
	IR_UMOD,                           // %t1 = umod %a, %b (unsigned)

	/* Bitwise */
	IR_AND,                            // %t1 = and %a, %b
	IR_OR,                             // %t1 = or %a, %b
	IR_XOR,                            // %t1 = xor %a, %b
	IR_SHL,                            // %t1 = shl %a, %b
	IR_SHR,                            // %t1 = shr %a, %b (logical)
	IR_SAR,                            // %t1 = sar %a, %b (arithmetic)
	IR_NOT,                            // %t1 = not %a, %b (bitwise)

	/* Unary */
	IR_NEG,                            // %t1 = neg %a

	/* Comparison */
	IR_EQ,                             // %t1 = eq %a, %b
    IR_NE,                             // %t1 = ne %a, %b
    IR_SLT,                            // %t1 = slt %a, %b (signed <)
    IR_SLE,                            // %t1 = sle %a, %b (signed <=)
    IR_SGT,                            // %t1 = sgt %a, %b (signed >)
    IR_SGE,                            // %t1 = sge %a, %b (signed >=)
    IR_ULT,                            // %t1 = ult %a, %b (unsigned <)
    IR_ULE,                            // %t1 = ule %a, %b (unsigned <=)
    IR_UGT,                            // %t1 = ugt %a, %b (unsigned >)
    IR_UGE,                            // %t1 = uge %a, %b (unsigned >=)

	/* Control Flow */
	IR_LABEL,                          // .L1:
	IR_JUMP,                           // jump .L1
	IR_BRANCH,                         // branch %cond, .L1, .L2
	IR_CALL,                           // call @func(%a, %b)
	IR_RETURN,                         // return %value (or void)

	/* Constants */
	IR_CONST_INT,                      // %t1 = const 42
	IR_CONST_FLOAT,                    // %t1 = const 3.14
	IR_CONST_STRING,                   // %t1 = const "hello"

	/* Type Conversion*/
	IR_SEXT,                           // %t1 = sext %a (sign extend)
    IR_ZEXT,                           // %t1 = zext %a (zero extend)
    IR_TRUNC,                          // %t1 = trunc %a
    IR_SITOFP,                         // %t1 = sitofp %a (int to float)
    IR_UITOFP,                         // %t1 = uitofp %a
    IR_FPTOSI,                         // %t1 = fptosi %a (float to int)
    IR_FPTOUI,                         // %t1 = fptoui %a
    IR_BITCAST,                        // %t1 = bitcast %a

	/* PHI Node (For SSA) */
	IR_PHI,                            // %t1 = phi [%a, .L1], [%b, .L2]
} IROp;

/* IR Value kinds */
typedef enum {
    IR_VALUE_TEMP,                     // SSA temporary: %t1, %t2
    IR_VALUE_GLOBAL,                   // Global variable: @x, @func
    IR_VALUE_CONST_INT,                // Integer constant: 42
    IR_VALUE_CONST_FLOAT,              // Float constant: 3.14
    IR_VALUE_CONST_STRING,             // String constant: "hello"
    IR_VALUE_LABEL,                    // Label: .L1, .L2
    IR_VALUE_UNDEF                     // Undefined value (for uninitialized vars)
} IRValueKind;

/* Forward declarations */
typedef struct IRValue IRValue;
typedef struct IRInstr IRInstr;
typedef struct IRBasicBlock IRBasicBlock;
typedef struct IRFunction IRFunction;
typedef struct IRModule IRModule;
typedef struct ResolvedType ResolvedType;

/* IR Value - represents operands and results */
struct IRValue {
    IRValueKind kind;
    ResolvedType *type;     // Type from semantic analysis
    
    union {
        int temp_id;        // For IR_VALUE_TEMP: %t42
        char *name;         // For IR_VALUE_GLOBAL: @func, @var
        long long int_val;  // For IR_VALUE_CONST_INT
        double float_val;   // For IR_VALUE_CONST_FLOAT
        char *string_val;   // For IR_VALUE_CONST_STRING
        int label_id;       // For IR_VALUE_LABEL: .L42
    };
};

typedef struct {
    char *name;
    IRValue *value;
} VarMapping;

/* PHI node incoming value (for SSA form) */
typedef struct {
    IRValue *value;         // The value
    IRBasicBlock *block;    // The predecessor block it comes from
} IRPhiIncoming;

/* IR Instruction - single operation */
struct IRInstr {
    IROp op;                // Operation type
    IRValue *dest;          // Result (NULL for void ops like STORE)
    
    union {
        /* For most operations */
        struct {
            IRValue *arg1;
            IRValue *arg2;
        };
        
        /* For CALL */
        struct {
            IRValue *callee;    // Function to call
            IRValue **args;     // Arguments
            size_t arg_count;
        } call;
        
        /* For BRANCH */
        struct {
            IRValue *cond;
            IRBasicBlock *true_block;
            IRBasicBlock *false_block;
        } branch;
        
        /* For JUMP */
        struct {
            IRBasicBlock *target;
        } jump;
        
        /* For RETURN */
        struct {
            IRValue *ret_value; // NULL for void return
        } ret;
        
        /* For PHI */
        struct {
            IRPhiIncoming *incoming;
            size_t incoming_count;
        } phi;
        
        /* For ALLOCA */
        struct {
            size_t size;        // Size in bytes
            size_t align;       // Alignment
        } alloca;
    };
    
    IRInstr *next;          // Next instruction (linked list)
};

/* Basic Block - sequence of instructions with single entry/exit */
struct IRBasicBlock {
    int id;                 // Block ID (.L1, .L2, etc.)
    char *name;             // Optional name for debugging
    
    IRInstr *first;         // First instruction
    IRInstr *last;          // Last instruction
    
    IRBasicBlock **preds;   // Predecessor blocks
    size_t pred_count;
    
    IRBasicBlock **succs;   // Successor blocks
    size_t succ_count;
    
    IRBasicBlock *next;     // Next block in function (linked list)
};

/* IR Function */
struct IRFunction {
    char *name;             // Function name
    ResolvedType *return_type; // Return type from SA
    
    IRValue **params;       // Function parameters
    size_t param_count;
    
    IRBasicBlock *entry;    // Entry block
    IRBasicBlock *blocks;   // All blocks (linked list)
    size_t block_count;
    
    int temp_counter;       // For generating %t1, %t2, ...
    int label_counter;      // For generating .L1, .L2, ...

	IRValue **values;
	size_t value_count;

    VarMapping *var_map;
    size_t var_map_count;
    
    IRFunction *next;       // Next function (linked list)
};

/* IR Module - top-level container */
struct IRModule {
    IRFunction *functions;  // All functions (linked list)
    size_t func_count;
    
    IRValue **globals;      // Global variables
    size_t global_count;

	IRValue **constants;
	size_t constant_count;
    
    char *source_file;      // Source filename for debugging
};

typedef struct LoopContext {
    IRBasicBlock *continue_target;
    IRBasicBlock *break_target;
    struct LoopContext *parent;
} LoopContext;
/* --- Globals ---*/

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

#endif