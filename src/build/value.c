
#include "../all.h"

Value* value_handle_idf(Allocator *alc, Parser* p, Idf *idf);
Value *value_func_call(Allocator *alc, Parser* p, Value *on);
Value* value_handle_class(Allocator *alc, Parser* p, Class* class);
Value* value_handle_ptrv(Allocator *alc, Parser* p);
Value* value_handle_compare(Allocator *alc, Parser* p, Value *left, Value* right, int op);
Value* pre_calc_float(Allocator* alc, Build* b, Value* n1, Value* n2, int op);
Value* pre_calc_int(Allocator* alc, Build* b, Value* n1, Value* n2, int op);

Value* read_value(Allocator* alc, Parser* p, bool allow_newline, int prio) {
    Build *b = p->b;
    Chunk *chunk = p->chunk;

    char t = tok(p, true, true, true);
    char* tkn = p->tkn;
    // char *tr = &chunk->token;

    Value* v = NULL;

    if (t == tok_at_word) {
        if (str_is(tkn, "@ptrv")) {
            v = value_handle_ptrv(alc, p);
        } else if (str_is(tkn, "@ptr_of")) {
            //
            tok_expect(p, "(", false, false);
            Value *from = read_value(alc, p, true, 0);
            if (!value_is_assignable(from)) {
                parse_err(p, -1, "Value in @ptr_of must be an assign-able value. e.g. a variable");
            }
            tok_expect(p, ")", true, true);
            if (from->type == v_decl) {
                Decl *decl = from->item;
                if (!decl->is_mut) {
                    decl->is_mut = true;
                }
            }
            v = value_make(alc, v_ptr_of, from, type_gen_volt(alc, b, "ptr"));

        } else if (str_is(tkn, "@stack")) {
            tok_expect(p, "(", false, false);
            Type* type = read_type(p, alc, true);
            tok_expect(p, ")", true, true);
            v = value_make(alc, v_stack, NULL, type);

        } else if (str_is(tkn, "@gc_link")){
            tok_expect(p, "(", false, false);
            Value* on = read_value(alc, p, true, 0);
            tok_expect(p, ",", true, true);
            Value* to = read_value(alc, p, true, 0);
            tok_expect(p, ")", true, true);
            if(type_is_gc(on->rett) && type_is_gc(to->rett)) {
                v = vgen_gc_link(alc, on, to, to->rett);
            } else {
                v = to;
            }
        } else if (str_is(tkn, "@gc_get_vtable")) {
            tok_expect(p, "(", false, false);
            Value* index = read_value(alc, p, true, 0);
            tok_expect(p, ")", true, true);
            v = value_make(alc, v_gc_get_table, index, type_gen_volt(alc, b, "ptr"));
        }
    } else if (t == tok_string) {
        char *body = tkn;
        body = string_replace_backslash_chars(b->alc, body);

        b->string_count++;
        char var[64];
        strcpy(var, "@.str.object.");
        itoa(b->string_count, (char *)((intptr_t)var + 13), 10);
        char *object_name = dups(b->alc, var);

        strcpy(var, "@.str.body.");
        itoa(b->string_count, (char *)((intptr_t)var + 11), 10);
        char *body_name = dups(b->alc, var);

        VString* str = al(b->alc, sizeof(VString));
        str->body = body;
        str->ir_object_name = object_name;
        str->ir_body_name = body_name;

        array_push(b->strings, str);

        v = value_make(alc, v_string, str, type_gen_volt(alc, b, "String"));

    } else if (t == tok_char) {

        v = vgen_int(alc, tkn[0], type_gen_volt(alc, b, "u8"));

    } else if (t == tok_bracket_open) {
        v = read_value(alc, p, true, 0);
        tok_expect(p, ")", true, true);
    } else if (t == tok_id) {
        if (str_is(tkn, "sizeof")) {
            tok_expect(p, "(", false, false);
            Type *type = read_type(p, alc, true);
            tok_expect(p, ")", true, true);
            v = vgen_int(alc, type->size, type_gen_volt(alc, b, "int"));
        } else if (str_is(tkn, "true") || str_is(tkn, "false")) {
            v = vgen_bool(alc, b, str_is(tkn, "true"));
        } else if (str_is(tkn, "null")) {
            v = value_make(alc, v_null, NULL, type_gen_null(alc, b));
        } else if (str_is(tkn, "atomic")) {
            tok_expect(p, "(", false, false);
            Value* val = read_value(alc, p, true, 0);
            tok_expect(p, ")", true, true);
            if(val->type != v_op) {
                parse_err(p, -1, "Value is not a math operation. e.g. (my_var + 2)");
            }
            VOp* vop = val->item;
            Value* left = vop->left;
            Value* right = vop->right;
            int op = vop->op;
            if(op != op_add && op != op_sub && op != op_mul && op != op_div){
                parse_err(p, -1, "Atomic only allows: + - * /");
            }
            if(!value_is_assignable(left)) {
                parse_err(p, -1, "Left value must be an assignable value. e.g. a variable");
            }
            v = value_make(alc, v_atomic, vop, val->rett);
        } else {
            // Identifiers
            Id id;
            read_id(p, tkn, &id);
            Idf *idf = idf_by_id(p, p->scope, &id, true);
            v = value_handle_idf(alc, p, idf);
        }
    } else if (t == tok_number || t == tok_sub) {
        bool negative = false;
        if(t == tok_sub) {
            negative = true;
            t = tok(p, false, false, true);
            if(t != tok_number) {
                parse_err(p, -1, "Invalid negative number syntax: '%s'", p->tkn);
            }
        }
        char* num = p->tkn;
        long int iv = 0;
        iv = atol(num);
        if (negative){
            iv *= -1;
        }
        v = vgen_int(alc, iv, type_gen_volt(alc, b, "int"));
    } else if(t == tok_plusplus || t == tok_subsub) {
        bool incr = t == tok_plusplus;
        if(tok_next_is_whitespace(p)) {
            parse_err(p, -1, "Missing value after '%s' (do not use whitespace)", incr ? "++" : "--");
        }
        Value* on = read_value(alc, p, false, 1);
        if(on->rett->type != type_int) {
            parse_err(p, -1, "Invalid value after '%s' (not an integer value)", incr ? "++" : "--");
        }
        if(!value_is_assignable(on)) {
            parse_err(p, -1, "Invalid value after '%s' (non-assign-able value)", incr ? "++" : "--");
        }
        value_is_mutable(on);
        v = vgen_incr(alc, b, on, incr, true);
    }

    if(!v) {
        parse_err(p, -1, "Unknown value: '%s'", tkn);
    }

    ///////////////////////
    // TRAILING CHARS
    ///////////////////////

    t = tok(p, false, false, false);
    while(t == tok_dot || t == tok_bracket_open || t == tok_plusplus || t == tok_subsub) {
        tok(p, false, false, true);
        Type* rett = v->rett;
        if(t == tok_dot) {
            Class *class = rett->class;
            if (!class) {
                parse_err(p, -1, "Unexpected '.'");
            }

            t = tok(p, false, false, true);
            char* prop_name = p->tkn;
            ClassProp* prop = map_get(class->props, prop_name);
            if(prop) {
                // Property
                v = vgen_class_pa(alc, v, prop);
            } else {
                // Check functions
                Func* func = map_get(class->funcs, prop_name);
                if (!func) {
                    parse_err(p, -1, "Class '%s' has no property/function named: '%s'", class->name, prop_name);
                }
                if(func->is_static) {
                    parse_err(p, -1, "Accessing a static class in a non-static way: '%s.%s'\n", class->name, prop_name);
                }
                v = vgen_func_ptr(alc, func, v);
            }
            t = tok(p, false, false, false);
            continue;
        }
        if(t == tok_bracket_open) {
            v = value_func_call(alc, p, v);
            t = tok(p, false, false, false);
            continue;
        }
        if(t == tok_plusplus || t == tok_subsub) {
            bool incr = t == tok_plusplus;
            if (v->rett->type != type_int) {
                parse_err(p, -1, "Invalid value before '%s' (not an integer value)", incr ? "++" : "--");
            }
            if (!value_is_assignable(v)) {
                parse_err(p, -1, "Invalid value before '%s' (non-assign-able value)", incr ? "++" : "--");
            }
            value_is_mutable(v);
            v = vgen_incr(alc, b, v, incr, false);
            t = tok(p, false, false, false);
            continue;
        }
        break;
    }

    ///////////////////////
    // TRAILING WORDS & OPS
    ///////////////////////

    t = tok(p, true, true, false);
    while (t == tok_at_word && str_is(p->tkn, "@as")) {
        tok(p, true, true, true);

        if (type_is_void(v->rett)) {
            parse_err(p, -1, "Left side of '@as' must return a value");
        }

        Type *type = read_type(p, alc, true);
        v = vgen_cast(alc, v, type);

        t = tok(p, true, true, false);
    }

    if (prio == 0 || prio > 10) {
        while (t == tok_mul || t == tok_div || t == tok_mod) {
            tok(p, true, true, true);
            int op;
            if (t == tok_mul)
                op = op_mul;
            else if (t == tok_div)
                op = op_div;
            else if (t == tok_mod)
                op = op_mod;
            else
                break;
            Value *right = read_value(alc, p, true, 10);
            v = value_handle_op(alc, p, v, right, op);
            t = tok(p, true, true, false);
        }
    }

    if (prio == 0 || prio > 20) {
        while (t == tok_plus || t == tok_sub) {
            tok(p, true, true, true);
            int op;
            if (t == tok_plus)
                op = op_add;
            else if (t == tok_sub)
                op = op_sub;
            else
                break;
            Value *right = read_value(alc, p, true, 20);
            v = value_handle_op(alc, p, v, right, op);
            t = tok(p, true, true, false);
        }
    }

    if (prio == 0 || prio > 25) {
        while (t == tok_shl || t == tok_shr) {
            tok(p, true, true, true);
            int op;
            if (t == tok_shl)
                op = op_shl;
            else if (t == tok_shr)
                op = op_shr;
            else
                break;
            Value *right = read_value(alc, p, true, 25);
            v = value_handle_op(alc, p, v, right, op);
            t = tok(p, true, true, false);
        }
    }

    if (prio == 0 || prio > 30) {
        while (t >= tok_eqeq && t <= tok_not_eq) {
            tok(p, true, true, true);
            int op;
            if(t == tok_eqeq)
                op = op_eq;
            else if(t == tok_not_eq)
                op = op_ne;
            else if(t == tok_lte)
                op = op_lte;
            else if(t == tok_gte)
                op = op_gte;
            else if(t == tok_lt)
                op = op_lt;
            else if(t == tok_gt)
                op = op_gt;
            else
                break;
            Value *right = read_value(alc, p, true, 30);
            v = value_handle_compare(alc, p, v, right, op);
            t = tok(p, true, true, false);
        }
    }

    if (prio == 0 || prio > 35) {
        while (t == tok_bit_and || t == tok_bit_or) {
            tok(p, true, true, true);
            int op;
            if(t == tok_bit_and)
                op = op_bit_and;
            else if(t == tok_bit_or)
                op = op_bit_or;
            else
                break;
            Value *right = read_value(alc, p, true, 35);
            v = value_handle_op(alc, p, v, right, op);
            t = tok(p, true, true, false);
        }
    }

    return v;
}

Value* value_handle_idf(Allocator *alc, Parser* p, Idf *idf) {
    Build *b = p->b;
    Chunk *chunk = p->chunk;

    int type = idf->type;

    if (type == idf_decl) {
        Decl* decl = idf->item;
        return value_make(alc, v_decl, decl, decl->type);
    }
    if (type == idf_global) {
        Global* g = idf->item;
        return value_make(alc, v_global, g, g->type);
    }
    if (type == idf_scope) {
        Scope* sub = idf->item;
        tok_expect(p, ":", false, false);
        char t = tok(p, false, false, true);

        Id id;
        read_id(p, p->tkn, &id);
        Idf *idf_sub = idf_by_id(p, sub, &id, true);
        return value_handle_idf(alc, p, idf_sub);
    }
    if (type == idf_func) {
        Func* func = idf->item;
        return vgen_func_ptr(alc, func, NULL);
    }
    if (type == idf_class) {
        Class* class = idf->item;
        return value_handle_class(alc, p, class);
    }
    if (type == idf_value_alias) {
        ValueAlias* va = idf->item;
        Chunk ch;
        ch = *p->chunk;
        *p->chunk = *va->chunk;
        Scope *scope = p->scope;
        p->scope = va->scope;
        Value* val = read_value(alc, p, true, 0);
        p->scope = scope;
        *p->chunk = ch;
        return val;
    }
    if (type == idf_cached_value) {
        return idf->item;
    }
    if (type == idf_value) {
        return idf->item;
    }

    parse_err(p, -1, "This identifier cannot be used inside a function. (identifier-type:%d)", idf->type);
    return NULL;
}

Value *value_func_call(Allocator *alc, Parser* p, Value *on) {
    Type* ont = on->rett;
    if (ont->type != type_func) {
        parse_err(p, -1, "Function call on non-function type");
    }
    
    Build* b = p->b;
    Array* func_args = ont->func_args;
    Array* func_default_values = ont->func_default_values;
    Type *rett = ont->func_rett;

    if (!func_args || !rett) {
        parse_err(p, -1, "Function pointer value is missing function type information (compiler bug)\n");
    }

    Array* args = array_make(alc, func_args->length + 1);
    int arg_i = 0;
    if(on->type == v_func_ptr) {
        VFuncPtr* fptr = on->item;
        if(fptr->first_arg) {
            arg_i++;
            array_push(args, fptr->first_arg);
        }
    }
    int offset = arg_i;
    bool contains_gc_args = false;

    // Read argument values
    if (tok(p, true, true, false) == tok_bracket_close) {
        tok(p, true, true, true);
    } else {
        while (true) {
            Value *arg = read_value(alc, p, true, 0);
            Type *arg_type = array_get_index(func_args, arg_i++);
            if (arg_type) {
                arg = try_convert(alc, b, arg, arg_type);
                type_check(p, arg_type, arg->rett);
            }

            array_push(args, arg);

            char t = tok(p, true, true, true);
            if (t == tok_comma)
                continue;
            if (t == tok_bracket_close)
                break;
            parse_err(p, -1, "Unexpected token in function arguments: '%s'\n", p->tkn);
        }
    }

    if(args->length > func_args->length) {
        parse_err(p, -1, "Too many arguments. Expected: %d, Found: %d\n", func_args->length - offset, args->length - offset);
    }

    // Add default values
    if (func_default_values) {
        int index = args->length;
        while (index < func_args->length) {
            Type *arg_type = array_get_index(func_args, index);
            Chunk *default_value_ch = array_get_index(func_default_values, index);
            if(!arg_type || !default_value_ch)
                break;
            Chunk ch;
            ch = *p->chunk;
            *p->chunk = *default_value_ch;
            Scope* scope = p->scope;
            p->scope = default_value_ch->fc->scope;
            //
            Value* arg = read_value(b->alc, p, true, 0);
            arg = try_convert(alc, b, arg, arg_type);
            type_check(p, arg_type, arg->rett);
            //
            p->scope = scope;
            *p->chunk = ch;
            array_push(args, arg);
            index++;
        }
    }

    if(args->length < func_args->length) {
        parse_err(p, -1, "Missing arguments. Expected: %d, Found: %d\n", func_args->length - offset, args->length - offset);
    }

    Value* fcall = vgen_func_call(alc, on, args);

    if(ont->func_errors) {
        VFuncCall* f = fcall->item;
        char t = tok_expect_two(p, "!", "?", true, false);

        if(t == tok_not) {

            t = tok(p, true, true, true);
            bool single = false;

            Chunk* chunk_end = NULL;
            if (t == tok_curly_open) {
                chunk_end = chunk_clone(alc, p->chunk);
                chunk_end->i = p->scope_end_i;
            } else if (t == tok_colon) {
                single = true;
            } else {
                parse_err(p, -1, "Expected '{' or ':' after '!', but found: '%s'", p->tkn);
            }

            Scope *scope = p->scope;
            Chunk *se = p->scope_end;
            Scope* err_scope = scope_sub_make(alc, sc_default, scope);
            f->err_scope = err_scope;

            p->scope = err_scope;
            p->scope_end = chunk_end;
            read_ast(p, single);
            p->scope = scope;
            p->scope_end = se;

            if(!err_scope->did_return) {
                parse_err(p, -1, "Expected scope to contain one of the following tokens: return, throw, break, continue");
            }

        } else if(t == tok_qmark) {
            Value* errv = read_value(alc, p, false, 0);
            errv = try_convert(alc, b, errv, fcall->rett);
            type_check(p, fcall->rett, errv->rett);
            f->err_value = errv;
        }
    }

    Value* buffer = vgen_gc_buffer(alc, b, p->scope, fcall, args, true);
    if(buffer->type == v_gc_buffer && p->scope->ast) {
        p->scope->gc_check = true;
    }
    return buffer;
}

Value* value_handle_class(Allocator *alc, Parser* p, Class* class) {
    Build* b = p->b;
    Chunk* ch = p->chunk;

    if(class->is_generic_base) {
        tok_expect(p, "[", false, false);
        Array* names = class->generic_names;
        Map* generic_types = map_make(alc);
        for (int i = 0; i < names->length; i++) {
            char* name = array_get_index(names, i);
            Type* type = read_type(p, alc, false);
            map_set(generic_types, name, type);
            if(i + 1 < names->length) {
                tok_expect(p, ",", true, false);
            } else {
                tok_expect(p, "]", true, false);
                break;
            }
        }
        class = get_generic_class(p, class, generic_types);
    }

    char t = tok(p, false, false, false);
    if(t == tok_dot) {
        // Static functions
        tok(p, false, false, true);
        t = tok(p, false, false, true);
        char* name = p->tkn;
        Func* func = map_get(class->funcs, name);
        if(!func) {
            parse_err(p, -1, "Class '%s' has no function named: '%s'", class->name, name);
        }
        return vgen_func_ptr(alc, func, NULL);
    }
    // Class init
    t = tok(p, true, false, false);
    if(t != tok_curly_open) {
        return NULL;
    }
    t = tok(p, true, false, true);
    int propc = class->props->values->length;
    Map* values = map_make(alc);
    // Read values
    t = tok(p, true, true, true);
    while(t != tok_curly_close) {
        char* name = p->tkn;
        ClassProp* prop = map_get(class->props, name);
        if(!prop) {
            parse_err(p, -1, "Class '%s' has no property named: '%s'", class->name, name);
        }
        if(map_contains(values, name)) {
            parse_err(p, -1, "Setting same property twice: '%s'", name);
        }
        tok_expect(p, ":", true, false);
        Value* val = read_value(alc, p, true, 0);
        val = try_convert(alc, b, val, prop->type);
        type_check(p, prop->type, val->rett);
        map_set_force_new(values, name, val);
        t = tok(p, true, true, true);
        if(t == tok_comma) {
            t = tok(p, true, true, true);
        }
    }
    // Default values
    Array* props = class->props->values;
    for(int i = 0; i < propc; i++) {
        ClassProp* prop = array_get_index(props, i);
        char* name = array_get_index(class->props->keys, i);
        if(!map_contains(values, name)) {
            if(prop->skip_default_value)
                continue;
            if(!prop->chunk_value) {
                parse_err(p, -1, "Missing property value for: '%s'", name);
            }

            Value* val = read_value_from_other_chunk(p, alc, prop->chunk_value, class->scope);

            val = try_convert(alc, b, val, prop->type);
            type_check(p, prop->type, val->rett);

            map_set_force_new(values, name, val);
        }
    }

    Value* init = value_make(alc, v_class_init, values, type_gen_class(alc, class));
    Value* buffer = vgen_gc_buffer(alc, b, p->scope, init, values->values, false);
    if(class->type == ct_class && p->scope->ast) {
        p->scope->gc_check = true;
    }

    return buffer;
}

Value* value_handle_ptrv(Allocator *alc, Parser* p) {
    tok_expect(p, "(", false, false);
    // On
    Value *on = read_value(alc, p, true, 0);
    if (!on->rett->is_pointer) {
        parse_err(p, -1, "First argument of '@ptrv' must be a value of type 'ptr'");
    }
    // Type
    tok_expect(p, ",", true, true);
    Type *type = read_type(p, alc, true);
    if (type->type == type_void) {
        parse_err(p, -1, "You cannot use 'void' type in @ptrv");
    }
    // Index
    char t = tok(p, true, true, false);
    Value *index = NULL;
    if(t == tok_comma) {
        tok(p, true, true, true);
        index = read_value(alc, p, true, 0);
        if (index->rett->type != type_int) {
            parse_err(p, -1, "@ptrv index must be of type integer");
        }
    }
    tok_expect(p, ")", true, true);

    return vgen_ptrv(alc, p->b, on, type, index);
}

Value* value_handle_op(Allocator *alc, Parser* p, Value *left, Value* right, int op) {
    // Check type
    Build* b = p->b;
    Type* lt = left->rett;
    Type* rt = right->rett;

    if(!lt->class || !lt->class->allow_math) {
        parse_err(p, -1, "You cannot use operators on these values");
    }
    bool is_ptr = false;
    bool is_signed = lt->is_signed || rt->is_signed;
    if(lt->type == type_ptr) {
        is_ptr = true;
        left = vgen_cast(alc, left, type_gen_volt(alc, b, is_signed ? "int" : "uint"));
    }
    if(rt->type == type_ptr) {
        is_ptr = true;
        right = vgen_cast(alc, right, type_gen_volt(alc, b, is_signed ? "int" : "uint"));
    }
    if(left->type == v_number && right->type != v_number) {
        try_convert_number(left, right->rett);
    } else if(right->type == v_number && left->type != v_number) {
        try_convert_number(right, left->rett);
    } else if(left->type == v_number && right->type == v_number) {
        // Pre-calculate
        bool is_float = left->rett->type == type_float || right->rett->type == type_float;
        Value* combo;
        if(is_float) {
            // combo = pre_calc_float(alc, fc->b, left, right, op);
        } else {
            combo = pre_calc_int(alc, b, left, right, op);
        }
        if(combo)
            return combo;
    }

    // Try match types
    match_value_types(alc, b, &left, &right);

    Type* t1 = left->rett;
    Type* t2 = right->rett;
    char* reason = NULL;
    if(!type_compat(t1, t2, &reason)){
        char t1b[256];
        char t2b[256];
        parse_err(p, -1, "Operator values are not compatible: %s %s %s", type_to_str(lt, t1b), op_to_str(op), type_to_str(rt, t2b));
    }

    Value* v = vgen_op(alc, op, left, right, t1);

    if(is_ptr) {
        v = vgen_cast(alc, v, type_gen_volt(alc, b, "ptr"));
    }

    return v;
}

Value* pre_calc_float(Allocator* alc, Build* b, Value* n1, Value* n2, int op) {
    VNumber* vn1 = n1->item;
    VNumber* vn2 = n2->item;
    double v1 = n1->rett->type == type_int ? (double) vn1->value_int : vn1->value_float;
    double v2 = n2->rett->type == type_int ? (double) vn2->value_int : vn2->value_float;
    if(op == op_add) {
        v1 = v1 + v2;
    } else if(op == op_sub) {
        v1 = v1 - v2;
    } else if(op == op_mul) {
        v1 = v1 * v2;
    } else if(op == op_div) {
        v1 = v1 / v2;
    } else {
        return NULL;
    }
    int size = max_num(n1->rett->size, n2->rett->size);
    return vgen_float(alc, v1, type_gen_number(alc, b, size, true, true));
}
Value* pre_calc_int(Allocator* alc, Build* b, Value* n1, Value* n2, int op) {
    VNumber* vn1 = n1->item;
    VNumber* vn2 = n2->item;
    int v1 = vn1->value_int;
    int v2 = vn2->value_int;
    if(op == op_add) {
        v1 = v1 + v2;
    } else if(op == op_sub) {
        v1 = v1 - v2;
    } else if(op == op_mul) {
        v1 = v1 * v2;
    } else if(op == op_div) {
        v1 = v1 / v2;
    } else {
        return NULL;
    }
    int size = max_num(n1->rett->size, n2->rett->size);
    return vgen_int(alc, v1, type_gen_number(alc, b, size, false, n1->rett->is_signed || n2->rett->is_signed));
}

Value* value_handle_compare(Allocator *alc, Parser* p, Value *left, Value* right, int op) {
    Build* b = p->b;
    Type* lt = left->rett;
    Type* rt = right->rett;

    bool is_ptr = lt->is_pointer || rt->is_pointer;
    if(is_ptr) {
        // Pointers
        if(lt->type == type_int) {
            left = vgen_cast(alc, left, type_gen_volt(alc, b, "ptr"));
        }
        if(rt->type == type_int) {
            right = vgen_cast(alc, right, type_gen_volt(alc, b, "ptr"));
        }
    } else {
        // Numbers
        bool is_bool = lt->type == type_bool || rt->type == type_bool;
        if (!is_bool) {
            if (left->type == v_number && right->type != v_number) {
                try_convert_number(left, rt);
            } else if (right->type == v_number && left->type != v_number) {
                try_convert_number(right, lt);
            }
            match_value_types(alc, b, &left, &right);
        }
    }

    Type *t1 = left->rett;
    Type* t2 = right->rett;
    char* reason = NULL;
    if(!type_compat(t1, t2, &reason)){
        char t1b[256];
        char t2b[256];
        parse_err(p, -1, "Operator values are not compatible: %s <-> %s", type_to_str(lt, t1b), type_to_str(rt, t2b));
    }

    return vgen_comp(alc, op, left, right, type_gen_volt(alc, b, "bool"));
}

bool value_is_assignable(Value *v) {
    if(v->type == v_ir_cached) {
        VIRCached* vc = v->item;
        return value_is_assignable(vc->value);
    }
    return v->type == v_decl || v->type == v_class_pa || v->type == v_ptrv || v->type == v_global; 
}

void match_value_types(Allocator* alc, Build* b, Value** v1_, Value** v2_) {
    //
    Value* v1 = *v1_;
    Value* v2 = *v2_;
    Type* t1 = v1->rett;
    Type* t2 = v2->rett;
    bool is_signed = t1->is_signed || t2->is_signed;
    bool is_float = t1->type == type_float || t2->type == type_float;
    int size = is_float ? max_num(t1->size, t2->size) : (max_num(t1->is_signed != is_signed ? t1->size * 2 : t1->size, t2->is_signed != is_signed ? t2->size * 2 : t2->size));
    Type* type = type_gen_number(alc, b, size, is_float, is_signed);
    if(!type)
        return;
    if(t1->type != type->type || t1->is_signed != type->is_signed || t1->size != type->size) {
        *v1_ = vgen_cast(alc, v1, type);
    }
    if(t2->type != type->type || t2->is_signed != type->is_signed || t2->size != type->size) {
        *v2_ = vgen_cast(alc, v2, type);
    }
}

void value_is_mutable(Value* v) {
    if (v->type == v_decl) {
        Decl *decl = v->item;
        decl->is_mut = true;
    }
}

Value* try_convert(Allocator* alc, Build* b, Value* val, Type* type) {
    // If number literal, change the type
    if(val->type == v_number && (type->type == type_int || type->type == type_float)){
        try_convert_number(val, type);
    }

    // If number value, cast if appropriate
    if(val->type != v_number){
        if (val->rett->type == type_int && type->type == type_float) {
            // We can always convert to float
            val->rett = type_clone(alc, type);
        } else if (val->rett->type == type_float && type->type == type_float) {
            // f32 -> f64
            if(val->rett->size < type->size) {
                val = vgen_cast(alc, val, type);
            }
        } else if (val->rett->type == type_int && type->type == type_int) {
            if(val->rett->is_signed && !type->is_signed) {
                // Not possible
            } else {
                int from_size = val->rett->size;
                int to_size = type->size;
                if(type->is_signed && !val->rett->is_signed) {
                    from_size *= 2;
                }
                if (from_size <= to_size) {
                    val = vgen_cast(alc, val, type);
                }
            }
        }
    }

    // If to ptr, just change value return type to ptr
    Class* ptr = get_volt_class(b, "type", "ptr");
    if (val->rett->class != ptr && type->class == ptr) {
        bool nullable = val->rett->nullable;
        val->rett = type_gen_class(alc, ptr);
        val->rett->nullable = nullable;
    }
    return val;
}

bool try_auto_cast_number(Value* val, Type* type) {
}

bool try_convert_number(Value* val, Type* type) {
    if(val->type != v_number || (type->type != type_int && type->type != type_float))
        return false;
    int bytes = type->size;
    int bits = bytes * 8;
    VNumber *number = val->item;
    if (type->type == type_int && val->rett->type != type_float) {
        long int one = 1;
        long int max = bytes < sizeof(intptr_t) ? (one << (bits - 1)) : INTPTR_MAX;
        long int min = 0;
        if (type->is_signed) {
            min = max * -1;
        }
        long int value = number->value_int;
        // printf("try:%ld\n", value);
        if (value >= min && value <= max) {
            // printf("conv:%ld\n", value);
            val->rett = type;
            return true;
        }
    } else if (type->type == type_float) {
        // TODO: float
    }
    return false;
}

bool value_needs_gc_buffer(Value* val) {
    if(!val->rett->is_pointer)
        return false;
    if(val->type == v_null)
        return false;
    if(val->type == v_decl)
        return false;
    return type_is_gc(val->rett);
}