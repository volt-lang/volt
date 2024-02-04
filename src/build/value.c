
#include "../all.h"

Value* value_handle_idf(Fc *fc, Scope *scope, Idf *idf);
Value *value_func_call(Allocator *alc, Fc *fc, Scope *scope, Value *on);


Value* read_value(Fc* fc, Scope* scope, bool allow_newline, int prio) {
    Allocator *alc = fc->alc_ast;
    Build *b = fc->b;
    Chunk *chunk = fc->chunk_parse;

    char *tkn = tok(fc, true, true, true);
    int t = chunk->token;

    Value* v = NULL;

    if (t == tok_id) {
        Idf *idf = read_idf(fc, scope, tkn, true);
        v = value_handle_idf(fc, scope, idf);
    }

    if(!v) {
        sprintf(b->char_buf, "Unknown value: '%s'", tkn);
        parse_err(chunk, b->char_buf);
    }

    while(tok_id_next(fc) == tok_char) {
        Type* rett = v->rett;
        char ch = tok_read_byte(fc, 1);
        if(ch == '.') {
            tok(fc, false, false, true);

            Class *class = rett->class;
            if (!class) {
                parse_err(chunk, "Unexpected '.'");
            }

            char* prop_name = tok(fc, false, false, true);
            ClassProp* prop = map_get(class->props, prop_name);
            if(prop) {
                // Property
                die("TODO: vgen-class-pa");
            } else {
                // Check functions
                Func* func = map_get(class->funcs, prop_name);
                if (!func) {
                    sprintf(b->char_buf, "Unknown class property/function: '%s'", tkn);
                    parse_err(chunk, b->char_buf);
                }
                die("TODO: vgen-class-func");
            }
            continue;
        }
        if(ch == '(') {
            tok(fc, false, false, true);
            v = value_func_call(alc, fc, scope, v);
            continue;
        }
        break;
    }
}

Value* value_handle_idf(Fc *fc, Scope *scope, Idf *idf) {
    Build *b = fc->b;
    Allocator *alc = fc->alc_ast;
    Chunk *chunk = fc->chunk_parse;

    int type = idf->type;

    if (type == idf_scope) {
        Scope* sub = idf->item;
        tok_expect(fc, ".", false, false);
        char *tkn = tok(fc, false, false, true);
        Idf *idf_sub = read_idf(fc, sub, tkn, true);
        return ast_handle_idf(fc, scope, idf_sub);
    }
    if (type == idf_func) {
        Func* func = idf->item;
        return vgen_func_ptr(alc, func, NULL);
    }

    sprintf(b->char_buf, "This identifier cannot be used inside a function. (identifier-type:%d)", idf->type);
    parse_err(chunk, b->char_buf);
}

Value *value_func_call(Allocator *alc, Fc *fc, Scope *scope, Value *on) {
    Type* ont = on->rett;
    if (ont->type != type_func) {
        parse_err(fc->chunk_parse, "Function call on non-function type");
    }
}

bool value_is_assignable(Value *v) {
    return v->type == v_decl || v->type == v_class_pa || v->type == v_ptrv || v->type == v_global;
}
