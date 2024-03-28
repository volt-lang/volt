
#ifndef H_IR
#define H_IR

#include "typedefs.h"

// Core
IR* ir_make(Unit* u, Parser* p);
void ir_gen_globals(IR* ir);
void ir_gen_functions(IR* ir);
void ir_gen_final(IR* ir);
// Type
char *ir_type(IR *ir, Type *type);
char *ir_type_int(IR *ir, int bytes);
char *ir_type_float(IR *ir, int bytes);
void ir_define_struct(IR *ir, Class* class);
char *ir_type_align(IR *ir, Type *type, char* result);
// Func
void ir_func_definition(Str* code, IR* ir, Func *vfunc, bool is_extern, Array* rett_refs);
void ir_define_ext_func(IR* ir, Func* func);
char *ir_alloca(IR *ir, IRFunc* func, Type *type);
char *ir_alloca_by_size(IR *ir, IRFunc* func, char* size);
void ir_func_return_nothing(IR* ir);
void ir_func_return(IR* ir, char* type, char* value);
char* ir_func_err_handler(IR* ir, Scope* scope, char* res, VFuncCall* fcall);
// Block
IRBlock *ir_block_make(IR *ir, IRFunc* func, char* prefix);
// AST
void ir_write_ast(IR* ir, Scope* scope);
char* ir_gc_link(IR* ir, char* on, char* to);
// Global
void ir_gen_globals(IR* ir);
void *ir_global(IR *ir, Global *g);
// Value
char* ir_value(IR* ir, Scope* scope, Value* v);
char* ir_assign_value(IR* ir, Scope* scope, Value* v);
// Generate
char *ir_var(IRFunc* func);
void ir_jump(IR* ir, IRBlock* block);
void ir_cond_jump(IR* ir, char* cond, IRBlock* block_if, IRBlock* block_else);
char *ir_int(IR* ir, v_i64 value);
char *ir_float(IR* ir, double value);
Array *ir_fcall_args(IR *ir, Scope *scope, Array *values, Array* rett_refs);
Array *ir_fcall_ir_args(IR *ir, Array *values, Array* types);
char *ir_func_call(IR *ir, char *on, Array *values, char *lrett, int line, int col);
char *ir_func_ptr(IR *ir, Func *func);
char *ir_string(IR *ir, VString *str);
char* ir_load(IR* ir, Type* type, char* var);
void ir_store(IR *ir, char *var, char *val, char* type, int type_size);
void ir_store_old(IR *ir, Type *type, char *var, char *val);
char *ir_cast(IR *ir, char *lval, Type *from_type, Type *to_type);
char *ir_i1_cast(IR *ir, char *val);
char* ir_op(IR* ir, Scope* scope, int op, char* left, char* right, Type* rett);
char* ir_compare(IR* ir, int op, char* left, char* right, char* type, bool is_signed, bool is_float);
char *ir_class_pa(IR *ir, Class *class, char *on, ClassProp *prop);
void ir_if(IR *ir, Scope *scope, TIf *ift);
void ir_while(IR *ir, Scope *scope, TWhile *item);
char* ir_ptrv(IR* ir, char* on, char* type, int index);
char* ir_ptr_offset(IR* ir, char* on, char* index, char* index_type, int size);
char* ir_ptrv_dyn(IR* ir, char* on, char* type, char* index, char* index_type);
char* ir_this_or_that(IR* ir, char* this, IRBlock* this_block, char* that, IRBlock* that_block, char* type);
char* ir_this_or_that_or_that(IR* ir, char* this, IRBlock* this_block, char* that, IRBlock* that_block, char* that2, IRBlock* that_block2, char* type);
char* ir_phi(IR* ir, Array* values, char* type);
char *ir_notnull_i1(IR *ir, char *val);
char *ir_and_or(IR *ir, IRBlock* block_current, char *left, IRBlock* block_right, char* right, IRBlock* block_last, int op);

// Structs
struct IR {
    Unit *unit;
    Build *b;
    Allocator* alc;
    Parser* parser;
    //
    char* char_buf;
    Str* str_buf;
    //
    Str* code_final;
    Str* code_struct;
    Str* code_global;
    Str* code_extern;
    Str* code_attr;
    //
    IRBlock* block;
    IRFunc* func;
    //
    IRBlock* block_after;
    IRBlock* block_cond;
    //
    IRBlock* vscope_after;
    Array* vscope_values;
    //
    Array* funcs;
    Array* attrs;
    Array* declared_funcs;
    Array* declared_classes;
    Array* declared_globals;
    // DI
    char *di_cu;
    char *di_file;
    char *di_retained_nodes;
    char *di_type_ptr;
    //
    int string_count;
    int attr_count;
    //
    bool use_stack_save;
    bool debug;
};
struct IRFunc {
    IR* ir;
    Func* func;
    //
    Array* blocks;
    IRBlock* block_start;
    IRBlock* block_code;
    //
    Array* rett_refs;
    //
    char* stack_save_vn;
    char* di_scope;
    char* gc_stack;
    char* gc_stack_adr;
    char* gc_stack_adr_val;
    //
    int var_count;
    int gc_count;
};
struct IRBlock {
    char* name;
    Str* code;
};
struct IRPhiValue {
    IRBlock* block;
    char* value;
};
struct IRFuncIR {
    Func* func;
    char* ir;
};

#endif
