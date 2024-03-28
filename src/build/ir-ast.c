
#include "../all.h"

void ir_write_ast(IR* ir, Scope* scope) {

    Allocator *alc = ir->alc;
    str_preserve(ir->block->code, 512);

    if(!scope->ast)
        return;

    Array *ast = scope->ast;
    for (int i = 0; i < ast->length; i++) {
        Token *t = array_get_index(ast, i);
        int tt = t->type;

        if (tt == t_statement) {
            Value *v = t->item;
            char *irv = ir_value(ir, scope, v);
            continue;
        }
        if (tt == t_declare) {
            TDeclare* item = t->item;
            Decl *decl = item->decl;
            Value *val = item->value;

            char *lval = ir_value(ir, scope, val);
            if (decl->is_mut) {
                ir_store_old(ir, decl->type, decl->ir_store_var, lval);
            } else {
                if(decl->is_gc) {
                    ir_store_old(ir, decl->type, decl->ir_store_var, lval);
                }
                decl->ir_var = lval;
            }
            continue;
        }
        if (tt == t_assign) {
            VPair* pair = t->item;
            Value* left = pair->left;
            Value* right = pair->right;
            char* value = ir_value(ir, scope, right);
            if (left->type == v_class_pa && type_is_gc(left->rett) && type_is_gc(right->rett)) {
                // GC link
                VClassPA *pa = left->item;
                Type* on_type = pa->on->rett;
                char* on = ir_value(ir, scope, pa->on);
                char* var = ir_class_pa(ir, on_type->class, on, pa->prop);
                char* result = ir_gc_link(ir, on, value);
                ir_store(ir, var, result, "ptr", ir->b->ptr_size);
            } else {
                char* var = ir_assign_value(ir, scope, left);
                ir_store_old(ir, left->rett, var, value);
            }
            continue;
        }

        if (tt == t_return) {
            Value *v = t->item;
            if(v) {
                char* irv = ir_value(ir, scope, v);
                ir_func_return(ir, ir_type(ir, v->rett), irv);
            } else {
                ir_func_return(ir, NULL, "void");
            }
            continue;
        }

        if (tt == t_if) {
            TIf *ift = t->item;
            ir_if(ir, scope, ift);
            continue;
        }

        if (tt == t_while) {
            TWhile *item = t->item;
            ir_while(ir, scope, item);
            continue;
        }
        if (tt == t_break) {
            Scope* loop_scope = t->item;
            IRBlock* after = ir->block_after;
            if(!after) {
                die("Missing IR after block for 'break' (compiler bug)");
            }
            ir_jump(ir, after);
            continue;
        }
        if (tt == t_continue) {
            Scope* loop_scope = t->item;
            IRBlock* cond = ir->block_cond;
            if(!cond) {
                die("Missing IR condition block for 'break' (compiler bug)");
            }
            ir_jump(ir, cond);
            continue;
        }
        if (tt == t_throw) {
            TThrow* tt = t->item;
            ir_store_old(ir, type_gen_valk(alc, ir->b, "i32"), "@valk_err_code", ir_int(ir, tt->err->value));
            char *msg = ir_string(ir, tt->msg);
            ir_store_old(ir, type_gen_valk(alc, ir->b, "ptr"), "@valk_err_msg", msg);
            ir_func_return_nothing(ir);
            continue;
        }
        if (tt == t_return_vscope) {
            char* val = ir_value(ir, scope, t->item);
            if(!ir->vscope_values) {
                die("Missing IR value-scope values array (compiler bug)");
            }
            IRPhiValue* v = al(ir->alc, sizeof(IRPhiValue));
            v->value = val;
            v->block = ir->block;
            array_push(ir->vscope_values, v);
            ir_jump(ir, ir->vscope_after);
            continue;
        }
        if (tt == t_set_var) {
            VVar *vv = t->item;
            vv->var = ir_value(ir, scope, vv->value);
            continue;
        }
        if (tt == t_ast_scope) {
            Scope *s = t->item;
            ir_write_ast(ir, s);
            continue;
        }
        if (tt == t_set_decl_store_var) {
            TDeclare* item = t->item;
            Decl* decl = item->decl;
            decl->ir_store_var = ir_assign_value(ir, scope, item->value);
            continue;
        }
        if (tt == t_set_return_value) {
            TSetRetv* sr = t->item;
            int index = sr->index;
            Value* val = sr->value;
            Array* rett_refs = ir->func->rett_refs;
            char* var = array_get_index(rett_refs, index);
            if(!var) {
                build_err(ir->b, "Missing return value IR variable (compiler bug)");
            }
            char* type = ir_type(ir, val->rett);
            IRBlock *block_if = ir_block_make(ir, ir->func, "if_set_ret_");
            IRBlock *after = ir_block_make(ir, ir->func, "set_ret_after_");
            char* nn = ir_notnull_i1(ir, var);
            ir_cond_jump(ir, nn, block_if, after);
            ir->block = block_if;
            ir_store(ir, var, ir_value(ir, scope, val), type, val->rett->size);
            ir_jump(ir, after);
            ir->block = after;
            continue;
        }
        if (tt == t_each) {
            TEach* item = t->item;
            char* on = ir_value(ir, scope, item->on);
            IRBlock *block_cond = ir_block_make(ir, ir->func, "each_cond_");
            IRBlock *block_code = ir_block_make(ir, ir->func, "each_code_");
            IRBlock *block_after = ir_block_make(ir, ir->func, "each_after_");
            ir_jump(ir, block_cond);
            ir->block = block_cond;

            Decl* kd = item->kd;
            Decl* kd_buf = item->kd_buf;
            Decl* vd = item->vd;
            Decl* index = item->index;

            Type* type_ptr = type_gen_valk(ir->alc, ir->b, "ptr");
            Array *types = array_make(ir->alc, 4);
            array_push(types, item->on->rett);
            array_push(types, index->type);
            array_push(types, type_ptr);
            Array *values = array_make(ir->alc, 4);
            array_push(values, on);
            array_push(values, ir_value(ir, scope, vgen_decl(alc, index)));
            array_push(values, ir_value(ir, scope, kd_buf ? value_make(alc, v_ptr_of, value_make(alc, v_decl, kd_buf, kd_buf->type), type_ptr) : vgen_null(alc, ir->b)));
            Array *args = ir_fcall_ir_args(ir, values, types);
            //
            char* fptr = ir_func_ptr(ir, item->func);
            char* fcall = ir_func_call(ir, fptr, args, ir_type(ir, item->func->rett), 0, 0);

            if(kd && !kd->is_mut) {
                kd->ir_var = ir_value(ir, scope, vgen_decl(alc, kd_buf));
            }
            if(vd->is_mut) {
                ir_store_old(ir, vd->type, vd->ir_store_var, fcall);
            } else {
                if (vd->is_gc) {
                    ir_store_old(ir, vd->type, vd->ir_store_var, fcall);
                }
                vd->ir_var = fcall;
            }
            // Increment index
            char* incr = ir_op(ir, scope, op_add, ir_value(ir, scope, vgen_decl(alc, index)), ir_int(ir, 1), index->type);
            ir_store(ir, ir_assign_value(ir, scope, vgen_decl(alc, index)), incr, ir_type(ir, index->type), index->type->size);
            // Cond
            Type *type_i32 = type_gen_valk(ir->alc, ir->b, "i32");
            char *load = ir_load(ir, type_i32, "@valk_err_code");
            char *lcond = ir_compare(ir, op_eq, load, "0", "i32", false, false);
            // Clear error
            ir_store_old(ir, type_i32, "@valk_err_code", "0");
            ir_cond_jump(ir, lcond, block_code, block_after);
            //
            ir->block = block_code;
            IRBlock *backup_after = ir->block_after;
            IRBlock *backup_cond = ir->block_cond;
            ir->block_after = block_after;
            ir->block_cond = block_cond;
            ir_write_ast(ir, item->scope);
            ir->block_after = backup_after;
            ir->block_cond = backup_cond;
            if(!item->scope->did_return) {
                ir_jump(ir, block_cond);
            }

            ir->block = block_after;
            // Clear from stack
            if (kd && kd->is_gc && kd->is_mut) {
                ir_store_old(ir, kd->type, kd->ir_store_var, "null");
            }
            if (kd_buf && kd_buf->is_gc && kd_buf->is_mut) {
                ir_store_old(ir, kd_buf->type, kd_buf->ir_store_var, "null");
            }
            if (vd->is_gc && vd->is_mut) {
                ir_store_old(ir, vd->type, vd->ir_store_var, "null");
            }
            //
            continue;
        }

        die("Unhandled IR token (compiler bug)");
    }
}

char* ir_gc_link(IR* ir, char* on, char* to) {
    Build* b = ir->b;

    Type* type_u8 = type_gen_valk(ir->alc, b, "u8");
    Type* type_ptr = type_gen_valk(ir->alc, b, "ptr");

    char* on_state_var = ir_ptrv(ir, on, "i8", -8);
    char* on_state = ir_load(ir, type_u8, on_state_var);

    IRBlock *current = ir->block;
    IRBlock *block_if = ir_block_make(ir, ir->func, "if_link_");
    IRBlock *block_link = ir_block_make(ir, ir->func, "link_");
    IRBlock *after = ir_block_make(ir, ir->func, "link_after_");

    // On state > transfer
    char *comp_on = ir_compare(ir, op_gt, on_state, "2", "i8", false, false);
    ir_cond_jump(ir, comp_on, block_if, after);

    // To state < solid
    ir->block = block_if;
    char* to_state_var = ir_ptrv(ir, to, "i8", -8);
    char* to_state = ir_load(ir, type_u8, to_state_var);

    char *comp_to = ir_compare(ir, op_lt, to_state, "4", "i8", false, false);
    ir_cond_jump(ir, comp_to, block_link, after);

    // Link
    ir->block = block_link;
    Func *func = get_valk_class_func(b, "mem", "Stack", "link");
    func_mark_used(ir->func->func, func);

    Value *fptr = vgen_func_ptr(ir->alc, func, NULL);
    //
    Array* types = array_make(ir->alc, 2);
    array_push(types, type_ptr);
    array_push(types, type_ptr);
    Array* values = array_make(ir->alc, 2);
    array_push(values, on);
    array_push(values, to);
    Array* args = ir_fcall_ir_args(ir, values, types);
    //
    char* link = ir_value(ir, NULL, fptr);
    char* link_rett = ir_func_call(ir, link, args, "ptr", 0, 0);
    ir_jump(ir, after);

    ir->block = after;
    char *result = ir_this_or_that_or_that(ir, to, current, to, block_if, link_rett, block_link, "ptr");
    return result;
}
