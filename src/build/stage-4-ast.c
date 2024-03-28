
#include "../all.h"

void stage_ast(Unit *u);

void stage_4_ast(Unit *u) {

    Build *b = u->b;

    if(u->is_main)
        return;

    usize start = microtime();

    stage_ast(u);

    b->time_parse += microtime() - start;

    stage_4_ir(u);
}

void stage_4_ast_main(Unit *u) {

    Build *b = u->b;

    usize start = microtime();
    stage_ast(u);
    b->time_parse += microtime() - start;

    stage_4_ir(u);
}

void stage_ast(Unit *u) {
    Build* b = u->b;
    Parser *p = u->parser;
    //
    Array *funcs = u->funcs;
    for (int i = 0; i < funcs->length; i++) {

        Func *func = array_get_index(funcs, i);

        if (func->in_header)
            continue;

        if (u->b->verbose > 2)
            printf("Stage 4 | Parse AST: %s\n", func->name);

        *p->chunk = *func->chunk_body;
        p->func = func;
        p->scope = func->scope;
        p->loop_scope = NULL;
        p->vscope_values = NULL;
        read_ast(p, false);

        if(p->cc_index > 0) {
            parse_err(p, -1, "Missing #end token");
        }
    }
}

void read_ast(Parser *p, bool single_line) {
    //
    Build* b = p->b;
    Allocator *alc = b->alc_ast;
    Scope* scope = p->scope;

    if (!scope->ast)
        scope->ast = array_make(alc, 50);

    bool first = false;

    while (true) {

        if (single_line && first)
            return;

        int before_i = p->chunk->i;
        char t = tok(p, true, true, true);
        char* tkn = p->tkn;

        if (tkn[0] == ';')
            continue;
        if (t == tok_curly_close)
            break;

        first = true;
        //
        if (t == tok_hashtag && p->on_newline) {
            cc_parse(p);
            continue;
        }

        if(scope->did_return)
            continue;

        if (t == tok_id) {
            if (str_is(tkn, "let")){
                Array *names = array_make(alc, 2);
                Array *types = array_make(alc, 2);
                while (true) {
                    t = tok(p, true, false, true);
                    char *name = p->tkn;
                    if (t != tok_id) {
                        parse_err(p, -1, "Invalid variable name: '%s'", name);
                    }
                    t = tok(p, true, false, true);
                    Type *type = NULL;
                    if (t == tok_colon) {
                        type = read_type(p, alc, true);
                        t = tok(p, true, false, true);
                    }
                    array_push(names, name);
                    array_push(types, type);

                    if(t != tok_comma)
                        break;
                }
                if(t != tok_eq) {
                    parse_err(p, -1, "Expected '=' here, found: '%s'", p->tkn);
                }

                Value* val = read_value(alc, p, true, 0);

                VFuncCall* fcall = value_extract_func_call(val);
                Array* fcall_rett_types = NULL;
                if(names->length > 1) {
                    if(!fcall) {
                        parse_err(p, -1, "Right side does not return multiple values");
                    }
                    Value* on = fcall->on;
                    fcall_rett_types = on->rett->func_rett_types;
                    if(!fcall_rett_types || fcall_rett_types->length < types->length) {
                        parse_err(p, -1, "Trying to declare %d variables, but the function only returns %d value(s)", types->length, fcall_rett_types ? fcall_rett_types->length : (type_is_void(on->rett->func_rett) ? 0 : 1));
                    }
                    fcall->rett_refs = array_make(alc, names->length);
                }
                for(int i = 0; i < names->length; i++) {
                    char *name = array_get_index(names, i);
                    Type *type = array_get_index(types, i);

                    if (!type) {
                        if(fcall_rett_types) {
                            type = array_get_index(fcall_rett_types, i);
                        } else {
                            type = val->rett;
                        }
                    }
                    if (i == 0) {
                        val = try_convert(alc, p, scope, val, type);
                        type_check(p, type, val->rett);
                    } else {
                        type_check(p, type, array_get_index(fcall_rett_types, i));
                    }

                    Decl *decl = decl_make(alc, name, type, false);
                    Idf *idf = idf_make(b->alc, idf_decl, decl);
                    scope_set_idf(scope, name, idf, p);

                    if(i == 0) {
                        array_push(scope->ast, tgen_declare(alc, scope, decl, val));
                    } else {
                        decl->is_mut = true;
                        scope_add_decl(alc, scope, decl);
                        array_push(fcall->rett_refs, value_make(alc, v_decl, decl, decl->type));
                    }
                }
                continue;
            }
            if (str_is(tkn, "if")){
                token_if(alc, p);
                continue;
            }
            if (str_is(tkn, "while")){
                token_while(alc, p);
                continue;
            }
            if (str_is(tkn, "break") || str_is(tkn, "continue")){
                if(!p->loop_scope) {
                    parse_err(p, -1, "Using 'break' without being inside a loop");
                }
                array_push(scope->ast, token_make(alc, str_is(tkn, "break") ? t_break : t_continue, p->loop_scope));
                scope->did_return = true;
                continue;
            }
            if (str_is(tkn, "return")){
                Value* val = NULL;

                if(p->vscope_values) {
                    // Value scope
                    val = read_value(alc, p, false, 0);

                    Array *values = p->vscope_values;
                    Type *type = vscope_get_result_type(values);
                    if(type) {
                        val = try_convert(alc, p, scope, val, type);
                        type_check(p, type, val->rett);
                    }
                    array_push(values, val);
                    array_push(scope->ast, token_make(alc, t_return_vscope, val));
                    scope->did_return = true;
                    continue;
                }

                Func* func = p->func;
                if(!func) {
                    parse_err(p, -1, "Using 'return' outside a function scope");
                }
                // Main return value
                if(!type_is_void(func->rett)) {
                    val = read_value(alc, p, false, 0);
                    val = try_convert(alc, p, p->scope, val, func->rett);
                    type_check(p, func->rett, val->rett);
                } else {
                    char t = tok(p, true, false, true);
                    if(t != tok_none && t != tok_semi && t != tok_curly_close) {
                        parse_err(p, -1, "Return statement should not return a value if the function has a 'void' return type");
                    }
                }
                if(val) {
                    // Important for GC
                    val = vgen_var(alc, b, val);
                    array_push(scope->ast, token_make(alc, t_set_var, val->item));
                }
                // Extra return values
                if(func->rett_types->length > 1) {
                    int extra_c = func->rett_types->length - 1;
                    int i = 1;
                    // Disable gc
                    Global* g_disable = get_valk_global(b, "mem", "disable_gc");
                    Value* disable = value_make(alc, v_global, g_disable, g_disable->type);
                    Value *var_disable = vgen_var(alc, b, disable);
                    array_push(scope->ast, token_make(alc, t_set_var, var_disable->item));
                    array_push(scope->ast, tgen_assign(alc, disable, vgen_bool(alc, b, true)));
                    while(i <= extra_c) {
                        tok_expect(p, ",", true, true);
                        Type* type = array_get_index(func->rett_types, i);
                        Value* val = read_value(alc, p, true, 0);
                        val = try_convert(alc, p, p->scope, val, type);
                        type_check(p, type, val->rett);
                        // Store
                        TSetRetv* sr = al(alc, sizeof(TSetRetv));
                        sr->index = i - 1;
                        sr->value = val;
                        array_push(scope->ast, token_make(alc, t_set_return_value, sr));
                        i++;
                    }
                    // Set disable_gc to previous value
                    array_push(scope->ast, tgen_assign(alc, disable, var_disable));
                }

                array_push(scope->ast, tgen_return(alc, val));
                scope->did_return = true;
                continue;
            }
            if (str_is(tkn, "throw")){
                char t = tok(p, true, false, true);
                if(t != tok_id) {
                    parse_err(p, -1, "Invalid error name: '%s'", p->tkn);
                }
                char* name = p->tkn;
                Func* func = p->func;
                Scope* fscope = func->scope;
                FuncError* err = NULL;
                if (func->errors) {
                    err = map_get(func->errors, name);
                }
                if(!err) {
                    parse_err(p, -1, "Function has no error defined named: '%s'", name);
                }

                array_push(scope->ast, tgen_throw(alc, b, p->unit, err, name));
                scope->did_return = true;
                continue;
            }
            if (str_is(tkn, "each")){
                Value* on = read_value(alc, p, true, 0);
                Func* func = on->rett->class ? map_get(on->rett->class->funcs, "_each") : NULL;
                if(!func) {
                    parse_err(p, -1, "Cannot use 'each' on this value. The class of it's type does not contain an '_each' method");
                }
                func_mark_used(p->func, func);

                tok_expect(p, "as", true, true);
                t = tok(p, true, false, true);
                if(t != tok_id) {
                    parse_err(p, -1, "Invalid variable name: '%s'", p->tkn);
                }
                char* kname = NULL;
                char* vname = p->tkn;
                t = tok(p, true, false, false);
                if(t == tok_comma) {
                    tok(p, true, false, true);
                    kname = vname;
                    t = tok(p, true, false, true);
                    if (t != tok_id) {
                        parse_err(p, -1, "Invalid variable name: '%s'", p->tkn);
                    }
                    vname = p->tkn;
                }
                if(kname && str_is(kname, vname)) {
                    parse_err(p, -1, "Key/value variable names cannot be the same: '%s'", kname);
                }

                char t = tok(p, true, true, true);
                int scope_end_i = -1;
                bool single = false;
                if (t == tok_curly_open) {
                    scope_end_i = p->scope_end_i;
                } else if (t == tok_colon) {
                    single = true;
                } else {
                    parse_err(p, -1, "Expected '{' or ':' at the end of the 'each' statement, but found: '%s'", p->tkn);
                }

                Scope *scope = p->scope;
                Scope *ls = p->loop_scope;
                Scope *scope_each = scope_sub_make(alc, sc_loop, scope);

                Decl *kd = NULL;
                if (kname) {
                    kd = decl_make(alc, kname, array_get_index(func->rett_types, 1), false);
                    Idf *idf = idf_make(b->alc, idf_decl, kd);
                    scope_set_idf(scope_each, kname, idf, p);
                    scope_add_decl(alc, scope, kd);
                }
                Decl *vd = decl_make(alc, vname, array_get_index(func->rett_types, 0), false);
                Idf *idf = idf_make(b->alc, idf_decl, vd);
                scope_set_idf(scope_each, vname, idf, p);
                scope_add_decl(alc, scope, vd);
                // Index
                Decl *index = decl_make(alc, NULL, type_gen_valk(alc, b, "uint"), false);
                index->is_mut = true;
                scope_add_decl(alc, scope, index);
                Value* vindex = value_make(alc, v_decl, index, index->type);
                // Set index to 0
                array_push(scope->ast, tgen_assign(alc, vindex, vgen_int(alc, 0, index->type)));

                p->scope = scope_each;
                p->loop_scope = scope_each;
                read_ast(p, single);
                p->scope = scope;
                p->loop_scope = ls;
                if (!single)
                    p->chunk->i = scope_end_i;

                array_push(scope->ast, tgen_each(alc, on, func, kd, vd, scope_each, index, vindex));
                continue;
            }
        }
        if (t == tok_at_word) {
            if (str_is(tkn, "@cache_value")){
                tok_expect(p, "(", false, false);
                Value* v = read_value(alc, p, true, 0);
                tok_expect(p, ")", true, true);
                tok_expect(p, "as", true, false);
                t = tok(p, true, false, true);
                if(t != tok_id) {
                    parse_err(p, -1, "Invalid variable name: '%s'", p->tkn);
                }
                char* name = p->tkn;
                Value* vc = vgen_ir_cached(alc, v);
                Idf* idf = idf_make(alc, idf_cached_value, vc);
                scope_set_idf(scope, name, idf, p);
                array_push(scope->ast, token_make(alc, t_statement, vc));
                continue;
            }
            if (str_is(tkn, "@gc_share")) {
                tok_expect(p, "(", false, false);
                Value* v = read_value(alc, p, true, 0);
                Type* rett = v->rett;
                if(!rett->class || (rett->class->type != ct_class && rett->class->type != ct_ptr) || !rett->is_pointer) {
                    parse_err(p, -1, "Invalid @gc_share value. It must be a pointer to an instance of a class");
                }
                tok_expect(p, ")", true, true);

                Func *share = get_valk_func(b, "mem", "gc_share");
                func_mark_used(p->func, share);
                Array* args = array_make(alc, 2);
                array_push(args, v);
                Value *on = vgen_func_ptr(alc, share, NULL);
                Value *fcall = vgen_func_call(alc, on, args);
                array_push(scope->ast, token_make(alc, t_statement, fcall));
                continue;
            }
            if (str_is(tkn, "@valk_gc_mark_globals")) {
                Func *func = b->func_mark_globals;
                Array* args = array_make(alc, 2);
                Value *on = vgen_func_ptr(alc, func, NULL);
                Value *fcall = vgen_func_call(alc, on, args);
                array_push(scope->ast, token_make(alc, t_statement, fcall));
                continue;
            }
            if (str_is(tkn, "@valk_gc_mark_shared")) {
                Func *func = b->func_mark_shared;
                Array* args = array_make(alc, 2);
                Value *on = vgen_func_ptr(alc, func, NULL);
                Value *fcall = vgen_func_call(alc, on, args);
                array_push(scope->ast, token_make(alc, t_statement, fcall));
                continue;
            }
        }

        p->chunk->i = before_i;

        Value* left = read_value(alc, p, true, 0);

        t = tok(p, true, false, false);
        if (t >= tok_eq && t <= tok_div_eq) {
            tok(p, true, false, true);
            if (!value_is_assignable(left)) {
                parse_err(p, -1, "Cannot assign to left side");
            }
            value_is_mutable(left);

            Value *right = read_value(alc, p, true, 0);
            if (type_is_void(right->rett)) {
                parse_err(p, -1, "Trying to assign a void value");
            }

            int op = op_eq;
            if (t == tok_eq) {
            } else if (t == tok_plus_eq) {
                op = op_add;
            } else if (t == tok_sub_eq) {
                op = op_sub;
            } else if (t == tok_mul_eq) {
                op = op_mul;
            } else if (t == tok_div_eq) {
                op = op_div;
            }
            if (op != op_eq) {
                right = value_handle_op(alc, p, left, right, op);
            }

            right = try_convert(alc, p, p->scope, right, left->rett);
            type_check(p, left->rett, right->rett);

            if(op == op_eq && left->type == v_global) {
                Global *g = left->item;
                if(g->is_shared && type_is_gc(g->type)) {
                    // Replace right with a temporary variable
                    right = vgen_var(alc, b, right);
                    array_push(scope->ast, token_make(alc, t_set_var, right->item));
                    // call gc_share(right)
                    Func *share = get_valk_func(b, "mem", "gc_share");
                    Array *args = array_make(alc, 2);
                    array_push(args, right);
                    Value *on = vgen_func_ptr(alc, share, NULL);
                    Value *fcall = vgen_func_call(alc, on, args);
                    array_push(scope->ast, token_make(alc, t_statement, fcall));
                }
            }

            array_push(scope->ast, tgen_assign(alc, left, right));
            continue;

        } else if (t == tok_comma) {
            // Multi assign
            Array* values = array_make(alc, 4);
            array_push(values, left);
            value_is_mutable(left);

            while(t == tok_comma) {
                tok(p, true, false, true);
                Value* v = read_value(alc, p, true, 0);
                array_push(values, v);
                t = tok(p, true, false, false);
            }

            tok_expect(p, "=", true, true);

            Value *val = read_value(alc, p, true, 0);

            VFuncCall *fcall = value_extract_func_call(val);
            if (!fcall) {
                parse_err(p, -1, "Right side does not return multiple values");
            }
            Value *on = fcall->on;
            Array *fcall_rett_types = on->rett->func_rett_types;
            if (!fcall_rett_types || fcall_rett_types->length < values->length) {
                parse_err(p, -1, "Trying to declare %d variables, but the function only returns %d value(s)", values->length, fcall_rett_types ? fcall_rett_types->length : (type_is_void(on->rett->func_rett) ? 0 : 1));
            }
            fcall->rett_refs = array_make(alc, values->length);

            for (int i = 0; i < values->length; i++) {
                Value *v = array_get_index(values, i);
                Type *type = v->rett;

                if (i == 0) {
                    val = try_convert(alc, p, scope, val, type);
                    type_check(p, type, val->rett);
                } else {
                    type_check(p, type, array_get_index(fcall_rett_types, i));
                }

                if (i == 0) {
                    array_push(scope->ast, tgen_assign(alc, v, val));
                } else {
                    array_push(fcall->rett_refs, v);
                }
            }
            continue;
        }

        //
        array_push(scope->ast, token_make(alc, t_statement, left));
    }

    if(scope->must_return && !scope->did_return) {
        parse_err(p, -1, "Missing return statement");
    }

    Scope* start;
    Scope* end;
    bool is_main_func_scope = p->func == b->func_main_gen && scope->type == sc_func;
    if (scope->has_gc_decls || scope->gc_check || is_main_func_scope) {
        // Start scope
        start = scope_sub_make(alc, sc_default, scope);
        start->ast = array_make(alc, 10);
        array_shift(scope->ast, token_make(alc, t_ast_scope, start));
        // End scope
        int last_token_index = scope->ast->length - 1;
        end = scope_sub_make(alc, sc_default, scope);
        end->ast = array_make(alc, 10);
        array_push(scope->ast, token_make(alc, t_ast_scope, end));
        // Swap return/continue/break token & end-scope
        if(scope->did_return) {
            int last_index = scope->ast->length - 1;
            void* a = array_get_index(scope->ast, last_token_index);
            void* b = array_get_index(scope->ast, last_index);
            array_set_index(scope->ast, last_token_index, b);
            array_set_index(scope->ast, last_index, a);
        }
    }

    if(is_main_func_scope) {
        Scope *boot = gen_snippet_ast(alc, p, get_valk_snippet(b, "mem", "boot"), map_make(alc), scope);
        array_push(start->ast, token_make(alc, t_ast_scope, boot));
    }

    if(scope->gc_check && (scope->type == sc_loop || scope->type == sc_func)){
        Scope *gcscope = gen_snippet_ast(alc, p, get_valk_snippet(b, "mem", "run_gc_check"), map_make(alc), scope);
        array_push(start->ast, token_make(alc, t_ast_scope, gcscope));
    }

    if (scope->has_gc_decls) {
        // Stack
        Map *idfs = map_make(alc);
        Value *amount = vgen_int(alc, scope->gc_decl_count, type_gen_number(alc, b, b->ptr_size, false, false));
        Idf *idf = idf_make(alc, idf_value, amount);
        map_set(idfs, "amount", idf);

        // Cache stack & stack_adr variable
        Scope *cache = gen_snippet_ast(alc, p, get_valk_snippet(b, "mem", "stack_cache"), idfs, start);
        array_push(start->ast, token_make(alc, t_ast_scope, cache));

        idf = map_get(cache->identifiers, "STACK_ADR");
        Value *stack_adr = idf->item;

        if (scope->type == sc_func) {
            // Stack reserve
            Scope *reserve = gen_snippet_ast(alc, p, get_valk_snippet(b, "mem", "stack_reserve"), idfs, start);
            array_push(start->ast, token_make(alc, t_ast_scope, reserve));

            // Set stack offset for varables
            Array *decls = scope->decls;
            int x = 0;
            for (int i = 0; i < decls->length; i++) {
                Decl *decl = array_get_index(decls, i);
                if (!decl->is_gc)
                    continue;
                Value *offset = vgen_ptrv(alc, b, stack_adr, type_gen_valk(alc, b, "ptr"), vgen_int(alc, x++, type_gen_valk(alc, b, "i32")));
                TDeclare *item = al(alc, sizeof(TDeclare));
                item->decl = decl;
                item->value = offset;
                array_push(start->ast, token_make(alc, t_set_decl_store_var, item));
                array_push(start->ast, tgen_assign(alc, value_make(alc, v_decl, decl, decl->type), vgen_null(alc, b)));
            }

            // Stack reduce
            Func* func = p->func;
            Scope* reduce_scope = scope_sub_make(alc, sc_default, scope);
            reduce_scope->ast = array_make(alc, 10);
            func->scope_stack_reduce = reduce_scope;
            Scope *scope_end = gen_snippet_ast(alc, p, get_valk_snippet(b, "mem", "stack_reduce"), idfs, scope);
            array_push(reduce_scope->ast, token_make(alc, t_ast_scope, scope_end));

        } else {
            // if / while : At end of scope, set local variables to null
            Array *decls = scope->decls;
            for (int i = 0; i < decls->length; i++) {
                Decl *decl = array_get_index(decls, i);
                array_push(end->ast, tgen_assign(alc, value_make(alc, v_decl, decl, decl->type), vgen_null(alc, b)));
            }
        }
    }
    //
    if(is_main_func_scope) {
        Scope *shutdown = gen_snippet_ast(alc, p, get_valk_snippet(b, "mem", "shutdown"), map_make(alc), scope);
        array_push(end->ast, token_make(alc, t_ast_scope, shutdown));
    }
}

