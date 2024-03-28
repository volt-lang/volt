
#ifndef _H_FUNC
#define _H_FUNC

#include "typedefs.h"

Func* func_make(Allocator* alc, Unit* u, Scope* parent, char* name, char* export_name);
FuncArg* func_arg_make(Allocator* alc, Type* type);
void func_mark_used(Func* func, Func* uses_func);
void parse_handle_func_args(Parser* p, Func* func);
void func_generate_args(Allocator* alc, Func* func, Map* args);
void func_validate_arg_count(Parser* p, Func* func, bool is_static, int arg_count_min, int arg_count_max);
void func_validate_rett_count(Parser* p, Func* func, bool is_static, int rett_count_min, int rett_count_max);
void func_validate_arg_type(Parser* p, Func* func, int index, Array* allowed_types);
void func_validate_rett(Parser* p, Func* func, Array* allowed_types);
void func_validate_rett_void(Parser *p, Func *func);

struct Func {
    char* name;
    char* export_name;
    Build *b;
    Unit* unit;
    Fc* fc;
    Scope* scope;
    Scope* scope_stack_reduce;
    //
    Chunk* chunk_args;
    Chunk* chunk_rett;
    Chunk* chunk_body;
    Chunk* body_end;
    //
    Map* args;
    Array* arg_types;
    Array* arg_values;
    Type* rett;
    Array* rett_types;
    Class* class;
    Array* cached_values;
    Map* errors;
    Array* used_functions;
    Array* used_classes;
    //
    int act;
    //
    bool is_static;
    bool is_inline;
    bool can_error;
    bool types_parsed;
    bool in_header;
    bool has_rett;
    bool multi_rett;
    bool is_test;
    bool is_used;
    bool use_if_class_is_used;
};
struct FuncArg {
    Type* type;
    Value* value;
    Chunk* chunk;
    Chunk* chunk_value;
    Decl* decl;
};
struct FuncError {
    ErrorCollection* collection;
    int value;
};
struct ErrorCollection {
    Map* errors;
};

#endif
