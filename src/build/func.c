
#include "../all.h"

Func* func_make(Allocator* alc, Unit* u, Scope* parent, char* name, char* export_name) {
    Func* f = al(alc, sizeof(Func));
    f->name = name;
    f->export_name = export_name;
    f->b = u->b;
    f->unit = u;
    f->act = act_public;
    f->fc = NULL;
    f->scope = scope_make(alc, sc_func, parent);
    f->scope_stack_reduce = NULL;
    f->chunk_args = NULL;
    f->chunk_rett = NULL;
    f->chunk_body = NULL;

    f->args = map_make(alc);
    f->arg_types = array_make(alc, 4);
    f->arg_values = array_make(alc, 4);
    f->rett_types = array_make(alc, 1);
    f->used_functions = array_make(alc, 4);
    f->used_classes = array_make(alc, 4);

    f->class = NULL;
    f->cached_values = NULL;
    f->errors = NULL;

    f->is_inline = false;
    f->is_static = false;
    f->can_error = false;
    f->types_parsed = false;
    f->in_header = false;
    f->has_rett = false;
    f->multi_rett = false;
    f->is_test = false;
    f->is_used = false;
    f->use_if_class_is_used = false;

    if (!export_name)
        f->export_name = gen_export_name(u->nsc, name);

    array_push(u->funcs, f);

    return f;
}

FuncArg* func_arg_make(Allocator* alc, Type* type) {
    FuncArg* a = al(alc, sizeof(FuncArg));
    a->type = type;
    a->value = NULL;
    a->chunk_value = NULL;
    return a;
}

void func_mark_used(Func* func, Func* uses_func) {
    if(!func)
        return;
    array_push(func->used_functions, uses_func);
}

void parse_handle_func_args(Parser* p, Func* func) {
    Build* b = p->b;

    tok_expect(p, "(", true, false);
    func->chunk_args = chunk_clone(b->alc, p->chunk);
    skip_body(p);

    // Return type
    func->chunk_rett = chunk_clone(b->alc, p->chunk);

    char t = tok(p, true, true, false);
    if (t != tok_curly_open && t != tok_not) {
        func->has_rett = true;
        if(t == tok_bracket_open) {
            func->multi_rett = true;
            skip_body(p);
        } else {
            skip_type(p);
        }
    }

    t = tok(p, true, true, false);
    Map *errors = NULL;
    if (t == tok_not) {
        errors = map_make(b->alc);
    }
    while(t == tok_not) {
        tok(p, true, true, true);
        t = tok(p, false, false, true);
        if(t != tok_id) {
            parse_err(p, -1, "Invalid error name: '%s'", p->tkn);
        }
        // Get error value
        char* name = p->tkn;
        FuncError* err = NULL;
        if(func->in_header) {
            build_err(b, "TODO: header errors");
        } else {
            err = map_get(b->errors->errors, name);
            if(!err) {
                err = al(b->alc, sizeof(FuncError));
                err->value = ++b->error_count;
                err->collection = b->errors;
                map_set(b->errors->errors, name, err);
            }
        }
        Idf* idf = idf_make(b->alc, idf_error, err);
        map_set(errors, name, err);
        scope_set_idf(func->scope, name, idf, p);

        t = tok(p, true, true, false);
    }
    func->errors = errors;

    if(!func->in_header) {
        tok_expect(p, "{", true, true);
        func->chunk_body = chunk_clone(b->alc, p->chunk);
        skip_body(p);
    } else {
        tok_expect(p, ";", true, true);
    }
}

char* ir_func_err_handler(IR* ir, Scope* scope, char* res, VFuncCall* fcall) {
    if(!fcall->err_scope && !fcall->err_value) {
        return res;
    }

    IRBlock *block_err = ir_block_make(ir, ir->func, "if_err_");
    IRBlock *block_else = fcall->err_value ? ir_block_make(ir, ir->func, "if_not_err_") : NULL;
    IRBlock *after = ir_block_make(ir, ir->func, "if_err_after_");

    Type* type_i32 = type_gen_valk(ir->alc, ir->b, "i32");
    char *load = ir_load(ir, type_i32, "@valk_err_code");
    char *lcond = ir_compare(ir, op_ne, load, "0", "i32", false, false);

    // Clear error
    ir_store_old(ir, type_i32, "@valk_err_code", "0");

    ir_cond_jump(ir, lcond, block_err, fcall->err_value ? block_else : after);

    if(fcall->err_scope) {

        Scope* err_scope = fcall->err_scope;
        ir->block = block_err;
        ir_write_ast(ir, err_scope);
        if(!err_scope->did_return) {
            ir_jump(ir, after);
        }
        ir->block = after;

        return res;

    } else if(fcall->err_value) {

        Value* val = fcall->err_value;
        char* ltype = ir_type(ir, val->rett);

        ir->block = block_err;
        char* alt_val = ir_value(ir, scope, val);
        ir_jump(ir, after);

        ir->block = block_else;
        ir_jump(ir, after);

        ir->block = after;
        Str* code = after->code;
        char* var = ir_var(ir->func);
        str_flat(code, "  ");
        str_add(code, var);
        str_flat(code, " = phi ");
        str_add(code, ltype);
        str_flat(code, " [ ");
        str_add(code, res);
        str_flat(code, ", %");
        str_add(code, block_else->name);
        str_flat(code, " ], [ ");
        str_add(code, alt_val);
        str_flat(code, ", %");
        str_add(code, block_err->name);
        str_flat(code, " ]\n");

        return var;
    }

    return NULL;
}


void func_generate_args(Allocator* alc, Func* func, Map* args) {
    int count = args->values->length;
    for(int i = 0; i < count; i++) {
        char* name = array_get_index(args->keys, i);
        Type* type = array_get_index(args->values, i);

        FuncArg *arg = func_arg_make(alc, type);
        map_set_force_new(func->args, name, arg);
        array_push(func->arg_types, arg->type);
        Decl *decl = decl_make(alc, name, arg->type, true);
        Idf *idf = idf_make(alc, idf_decl, decl);
        scope_set_idf(func->scope, name, idf, NULL);
        arg->decl = decl;
        array_push(func->arg_values, NULL);
    }
}

void func_validate_arg_count(Parser* p, Func* func, bool is_static, int arg_count_min, int arg_count_max) {

    if (func->is_static != is_static) {
        *p->chunk = *func->chunk_args;
        if (is_static)
            parse_err(p, -1, "Expected function to be static");
        else
            parse_err(p, -1, "Expected function to be non-static");
    }

    int argc = func->arg_types->length;
    int offset = !func->class || is_static ? 0 : 1;
    if (argc < arg_count_min) {
        *p->chunk = *func->chunk_args;
        parse_err(p, -1, "Expected amount of function arguments: %d, instead of: %d", arg_count_min - offset, argc - offset);
    }
    if (argc > arg_count_max) {
        *p->chunk = *func->chunk_args;
        parse_err(p, -1, "Expected amount of function arguments: %d, instead of: %d", arg_count_max - offset, argc - offset);
    }
}
void func_validate_rett_count(Parser* p, Func* func, bool is_static, int rett_count_min, int rett_count_max) {

    if (func->is_static != is_static) {
        *p->chunk = *func->chunk_args;
        if (is_static)
            parse_err(p, -1, "Expected function to be static");
        else
            parse_err(p, -1, "Expected function to be non-static");
    }

    int rettc = func->rett_types->length;
    if (rettc < rett_count_min) {
        *p->chunk = *func->chunk_rett;
        parse_err(p, -1, "Expected amount of function return-types: %d, instead of: %d", rett_count_min, rettc);
    }
    if (rettc > rett_count_max) {
        *p->chunk = *func->chunk_rett;
        parse_err(p, -1, "Expected amount of function return-types: %d, instead of: %d", rett_count_max, rettc);
    }
}
void func_validate_arg_type(Parser* p, Func* func, int index, Array* allowed_types) {
    bool has_valid = false;
    FuncArg* arg = array_get_index(func->args->values, index);
    Type *rett = arg->type;
    int count = allowed_types->length;
    for (int i = 0; i < count; i++) {
        Type *valid = array_get_index(allowed_types, i);
        if(rett->class == valid->class && rett->nullable == valid->nullable && rett->is_pointer == valid->is_pointer) {
            has_valid = true;
            break;
        }
    }
    if(!has_valid){
        *p->chunk = *arg->chunk;
        char types[2048];
        char buf[512];
        types[0] = 0;
        for (int i = 0; i < count; i++) {
            Type *valid = array_get_index(allowed_types, i);
            type_to_str(valid, buf);
            if(i > 0) {
                strcat(types, ", ");
            }
            strcat(types, buf);
        }
        type_to_str(rett, buf);
        parse_err(p, -1, "Function return type is '%s', but should be on of the following: %s", buf, types);
    }
}
void func_validate_rett(Parser* p, Func* func, Array* allowed_types) {
    bool has_valid = false;
    Type *rett = func->rett;
    int count = allowed_types->length;
    for (int i = 0; i < count; i++) {
        Type *valid = array_get_index(allowed_types, i);
        if(rett->class == valid->class && rett->nullable == valid->nullable && rett->is_pointer == valid->is_pointer) {
            has_valid = true;
            break;
        }
    }
    if(!has_valid){
        *p->chunk = *func->chunk_rett;
        char types[2048];
        char buf[512];
        types[0] = 0;
        for (int i = 0; i < count; i++) {
            Type *valid = array_get_index(allowed_types, i);
            type_to_str(valid, buf);
            if(i > 0) {
                strcat(types, ", ");
            }
            strcat(types, buf);
        }
        type_to_str(rett, buf);
        parse_err(p, -1, "Function return type is '%s', but should be on of the following: %s", buf, types);
    }
}

void func_validate_rett_void(Parser *p, Func *func) {
    if (!type_is_void(func->rett)) {
        *p->chunk = *func->chunk_rett;
        char buf[512];
        type_to_str(func->rett, buf);
        parse_err(p, -1, "Expected function return type to be 'void' instead of '%s'", buf);
    }
}