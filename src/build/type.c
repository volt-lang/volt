
#include "../all.h"

Type* type_make(Allocator* alc, int type) {
    Type* t = al(alc, sizeof(Type));
    t->type = type;
    t->size = 0;
    t->array_size = 0;
    t->class = NULL;
    t->func_args = NULL;
    t->func_default_values = NULL;
    t->func_errors = NULL;
    t->func_rett = NULL;
    t->func_rett_types = NULL;
    t->array_type = NULL;
    t->is_pointer = false;
    t->is_signed = false;
    t->nullable = false;
    t->func_can_error = false;
    return t;
}

Type* read_type(Parser* p, Allocator* alc, bool allow_newline) {
    //
    Build* b = p->b;
    Scope* scope = p->scope;

    Type *type = NULL;
    bool nullable = false;
    bool is_inline = false;

    char t = tok(p, true, allow_newline, true);

    if(t == tok_at_word) {
        if (str_is(p->tkn, "@ignu")) {
            tok_expect(p, "(", false, false);
            Type* type = read_type(p, alc, false);
            tok_expect(p, ")", true, false);
            type->nullable = false;
            type->ignore_null = true;
            return type;
        }
    }

    if(t == tok_qmark) {
        nullable = true;
        t = tok(p, false, false, true);
    }

    if(t == tok_id) {
        char* tkn = p->tkn;
        if (str_is(tkn, "void")) {
            return type_make(alc, type_void);
        }
        if (str_is(tkn, "fn")) {
            tok_expect(p, "(", false, false);
            // Args
            Array* args = array_make(alc, 4);
            while (tok(p, true, false, false) != tok_bracket_close) {
                Type* type = read_type(p, alc, false);
                array_push(args, type);
                if (tok(p, true, false, false) != tok_bracket_close) {
                    tok_expect(p, ",", true, false);
                }
            }
            tok_expect(p, ")", false, false);
            // Return types
            Array *return_types = array_make(alc, 2);
            tok_expect(p, "(", false, false);
            char tt = tok(p, true, false, false);
            if (tt == tok_bracket_close)
                tok(p, true, false, true);
            while (tt != tok_bracket_close) {
                Type* rett = read_type(p, alc, false);
                array_push(return_types, rett);
                tt = tok_expect_two(p, ",", ")", true, false);
            }
            Type* rett = array_get_index(return_types, 0);
            rett = rett ? rett : type_gen_void(alc);
            //
            Type *t = type_make(alc, type_func);
            t->func_rett = rett;
            t->func_rett_types = return_types;
            t->func_args = args;
            t->size = b->ptr_size;
            t->is_pointer = true;
            t->nullable = nullable;
            return t;
        }

        if (str_is(tkn, "inline")) {
            is_inline = true;
            t = tok(p, true, false, true);
            tkn = p->tkn;
        }

        if (t == tok_sq_bracket_open) {
            Type* array_type = read_type(p, alc, false);
            tok_expect(p, ",", true, false);
            t = tok(p, true, false, true);
            if(t != tok_number) {
                parse_err(p, -1, "Expected a valid number here to define the length of the static array");
            }
            int itemc = atoi(p->tkn);
            if(itemc == 0) {
                parse_err(p, -1, "Static array size must be larger than 0, size: '%s'", p->tkn);
            }
            tok_expect(p, "]", true, false);
            Type *t = type_make(alc, type_static_array);
            t->array_type = array_type;
            t->is_pointer = !is_inline;
            t->size = is_inline ? (array_type->size * itemc) : b->ptr_size;
            t->array_size = itemc;
            return t;
        }

        if (t == tok_id) {
            // Identifier
            Id id;
            read_id(p, tkn, &id);
            Idf *idf = idf_by_id(p, scope, &id, false);
            if (!idf) {
                id.ns ? parse_err(p, -1, "Unknown type: '%s:%s'", id.ns, id.name)
                      : parse_err(p, -1, "Unknown type: '%s'", id.name);
            }
            if (idf->type == idf_class) {
                Class *class = idf->item;

                if (class->is_generic_base) {
                    tok_expect(p, "[", false, false);
                    int count = class->generic_names->length;
                    Array* generic_types = array_make(alc, count + 1);
                    for (int i = 0; i < count; i++) {
                        Type* type = read_type(p, alc, false);
                        array_push(generic_types, type);
                        if(i + 1 < count) {
                            tok_expect(p, ",", true, false);
                        } else {
                            tok_expect(p, "]", true, false);
                            break;
                        }
                    }
                    class = get_generic_class(p, class, generic_types);
                }

                type = type_gen_class(alc, class);
            }
            if (idf->type == idf_type) {
                type = type_clone(alc, idf->item);
            }
        }
    }

    if (type) {
        if (is_inline) {
            type->is_pointer = false;
            if(type->type == type_struct) {
                type->size = type->class->size;
            }
        }
        if (nullable) {
            if (!type->is_pointer) {
                char buf[256];
                parse_err(p, -1, "This type cannot be null: '?%s'", type_to_str(type, buf));
            }
            type->nullable = true;
        }
        return type;
    }

    parse_err(p, -1, "Invalid type: '%s'", p->tkn);
    return NULL;
}
Type* type_clone(Allocator* alc, Type* type) {
    Type* t = al(alc, sizeof(Type));
    *t = *type;
    return t;
}

Type* type_gen_void(Allocator* alc) {
    return type_make(alc, type_void);
}
Type* type_gen_null(Allocator* alc, Build* b) {
    Type* t = type_make(alc, type_null);
    t->is_pointer = true;
    t->size = b->ptr_size;
    t->nullable = true;
    return t;
}
Type* type_gen_class(Allocator* alc, Class* class) {
    Type* t = type_make(alc, type_struct);
    t->class = class;
    t->size = class->b->ptr_size;

    const int ct = class->type;
    if(ct == ct_bool) {
        t->type = type_bool;
    } else if (ct == ct_ptr) {
        t->type = type_ptr;
    } else if (ct == ct_int) {
        t->type = type_int;
        t->size = class->size;
        t->is_signed = class->is_signed;
    } else if (ct == ct_float) {
        t->type = type_float;
    }

    if(ct == ct_class || ct == ct_struct || ct == ct_ptr)
        t->is_pointer = true;

    return t;
}
Type* type_gen_func(Allocator* alc, Func* func) {
    Type* t = type_make(alc, type_func);
    t->func_rett = func->rett;
    t->func_rett_types = func->rett_types;
    t->func_args = func->arg_types;
    t->func_default_values = func->arg_values;
    t->func_errors = func->errors;
    t->size = func->b->ptr_size;
    t->is_pointer = true;
    t->func_can_error = func->errors ? true : false;
    return t;
}
Type* type_gen_valk(Allocator* alc, Build* b, char* name) {
    if (name[0] == 'u' && str_is(name, "uint")) {
        name = get_number_type_name(b, b->ptr_size, false, false);
    } else if (name[0] == 'i' && str_is(name, "int")) {
        name = get_number_type_name(b, b->ptr_size, false, true);
    } else if (name[0] == 'f' && str_is(name, "float")) {
        name = get_number_type_name(b, b->ptr_size, true, false);
    }
    Nsc* nsc = get_valk_nsc(b, "type");
    Idf* idf = scope_find_idf(nsc->scope, name, false);
    if(idf && idf->type == idf_class) {
        return type_gen_class(alc, idf->item);
    }
    printf("VALK TYPE NOT FOUND: '%s'", name);
    exit(1);
}

char* get_number_type_name(Build* b, int size, bool is_float, bool is_signed) {
    if(is_float) {
        if(size == 4)
            return "f32";
        if(size == 8)
            return "f64";
    } else {
        if(size == 1)
            return is_signed ? "i8" : "u8";
        if(size == 2)
            return is_signed ? "i16" : "u16";
        if(size == 4)
            return is_signed ? "i32" : "u32";
        if(size == 8)
            return is_signed ? "i64" : "u64";
    }
    return NULL;
}

Type* type_gen_number(Allocator* alc, Build* b, int size, bool is_float, bool is_signed) {
    char* name = get_number_type_name(b, size, is_float, is_signed);
    if(name) {
        return type_gen_valk(alc, b, name);
    }
    return NULL;
}

Type* type_merge(Allocator* alc, Type* t1, Type* t2) {
    if (t1->class != t2->class)
        return NULL;
    if (t1->is_pointer != t2->is_pointer)
        return NULL;
    Type* type = type_make(alc, t1->type);
    *type = *t1;
    type->nullable = t1->nullable || t2->nullable;
    return type;
}

bool type_compat(Type* t1, Type* t2, char** reason) {
    if (t2->type == type_null && (t1->type == type_ptr || t1->nullable || t1->ignore_null)) {
        return true;
    }
    if (t1->type != t2->type) {
        *reason = "different kind of types";
        return false;
    }
    if (t1->is_signed != t2->is_signed) {
        *reason = "signed vs unsigned";
        return false;
    }
    if (t1->size != t2->size) {
        *reason = "types have different sizes";
        return false;
    }
    if (t1->is_pointer != t2->is_pointer) {
        *reason = "pointer vs no-pointer";
        return false;
    }
    if (t1->type == type_struct && t1->class != t2->class) {
        *reason = "different classes";
        return false;
    }
    if (!t1->ignore_null && t2->nullable && !t1->nullable) {
        *reason = "non-null vs null-able-type";
        return false;
    }
    if (t1->type == type_func) {
        Array* t1s = t1->func_args;
        Array* t2s = t2->func_args;
        if(t1s->length != t2s->length){
            *reason = "different amount of argument types";
            return false;
        }
        for(int i = 0; i < t1s->length; i++) {
            Type *ft1 = array_get_index(t1s, i);
            Type* ft2 = array_get_index(t2s, i);
            if (!type_compat(ft2, ft1, reason)) {
                *reason = "argument types not compatible";
                return false;
            }
        }
        // Return types
        t1s = t1->func_rett_types;
        t2s = t2->func_rett_types;
        if(t1s->length != t2s->length){
            *reason = "different amount of return types";
            return false;
        }
        for(int i = 0; i < t1s->length; i++) {
            Type *ft1 = array_get_index(t1s, i);
            Type* ft2 = array_get_index(t2s, i);
            if (!type_compat(ft1, ft2, reason)) {
                *reason = "return types not compatible";
                return false;
            }
        }
    }
    return true;
}
void type_check(Parser* p, Type* t1, Type* t2) {
    char* reason = NULL;
    if(!type_compat(t1, t2, &reason)) {
        char t1b[256];
        char t2b[256];
        parse_err(p, -1, "Types are not compatible | %s <-> %s | Reason: %s", type_to_str(t1, t1b), type_to_str(t2, t2b), reason ? reason : "?");
    }
}

bool type_is_void(Type* type) { return type->type == type_void; }
bool type_is_bool(Type* type) { return type->type == type_bool; }
bool type_is_gc(Type* type) { return type->is_pointer && type->type == type_struct && type->class->type == ct_class; }

void type_to_str_append(Type* t, char* res, bool use_export_name) {
    if (t->type == type_void) {
        strcat(res, "void");
        return;
    } else if (t->type == type_null) {
        strcat(res, "null");
        return;
    }
    if (t->nullable) {
        strcat(res, use_export_name ? "NULL_" : "?");
    }
    if (t->type == type_func) {
        strcat(res, use_export_name ? "fn_" : "fn(");
        Array * types = t->func_args;
        for(int i = 0; i < types->length; i++) {
            if(i > 0) {
                strcat(res, use_export_name ? "_" : ", ");
            }
            Type* sub = array_get_index(types, i);
            type_to_str_append(sub, res, use_export_name);
        }
        strcat(res, use_export_name ? "__" : ")(");
        types = t->func_rett_types;
        for(int i = 0; i < types->length; i++) {
            if(i > 0) {
                strcat(res, use_export_name ? "_" : ", ");
            }
            Type* sub = array_get_index(types, i);
            type_to_str_append(sub, res, use_export_name);
        }
        strcat(res, use_export_name ? "_" : ")");
    } else if (t->class) {
        Class *class = t->class;
        strcat(res, use_export_name ? class->ir_name : class->name);
    }
}
char* type_to_str(Type* t, char* res) {
    strcpy(res, "");
    type_to_str_append(t, res, false);
    return res;
}
char* type_to_str_export(Type* t, char* res) {
    strcpy(res, "");
    type_to_str_append(t, res, true);
    return res;
}

void type_to_str_buf_append(Type* t, Str* buf) {
    char* tmp = ((char*)(buf->data)) + buf->length;
    tmp[0] = 0;
    type_to_str_append(t, tmp, true);
    buf->length += strlen(tmp);
}

int type_get_size(Build* b, Type* type) {
    if (type->size > 0) {
        return type->size;
    }
    if (type->is_pointer) {
        return b->ptr_size;
    } else if (type->class) {
        return type->class->size;
    }
    return -1;
}

Array* gen_type_array_1(Allocator* alc, Build* b, char* type1, bool nullable) {
    Array* types = array_make(alc, 2);
    Type* t1 = type_gen_valk(alc, b, type1);
    t1->nullable = nullable;
    array_push(types, t1);
    return types;
}
Array* gen_type_array_2(Allocator* alc, Build* b, char* type1, bool nullable1, char* type2, bool nullable2) {
    Array* types = array_make(alc, 2);

    Type* t1 = type1 ? type_gen_valk(alc, b, type1) : type_gen_void(alc);
    t1->nullable = nullable1;
    array_push(types, t1);

    Type* t2 = type2 ? type_gen_valk(alc, b, type2) : type_gen_void(alc);
    t2->nullable = nullable2;
    array_push(types, t2);

    return types;
}


Type* vscope_get_result_type(Array* values) {
    Value *first = array_get_index(values, 0);
    if(!first)
        return NULL;
    Type *type = first->rett;
    bool contains_nullable = false;
    for (int i = 0; i < values->length; i++) {
        Value *v = array_get_index(values, i);
        if (v->rett->nullable) {
            contains_nullable = true;
            break;
        }
    }
    if (type->is_pointer)
        type->nullable = contains_nullable;
    return type;
}
