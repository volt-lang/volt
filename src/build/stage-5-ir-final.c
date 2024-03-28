
#include "../all.h"

void stage_5_mark_used_rec(Func* func);
void stage_5_mark_class_used(Class* class);
void stage_5_vtable_ir(Str* code, Build* b);
char* ir_vtable_func_name(Func* func, char* buf);

void stage_5_ir_final(Build* b) {

    // Mark main func as used (recursive)
    if(b->func_main)
        stage_5_mark_used_rec(b->func_main);
    stage_5_mark_used_rec(b->func_mark_globals);
    stage_5_mark_used_rec(b->func_mark_shared);
    stage_5_mark_used_rec(b->func_main_gen);

    Func *gac_func = get_valk_func(b, "mem", "gc_alloc_class");
    stage_5_mark_used_rec(gac_func);

    Str *code = str_make(b->alc, 50000);
    Str *hash_buf = str_make(b->alc, 100);

    Array *units = b->units;
    for (int i = 0; i < units->length; i++) {
        Unit *u = array_get_index(units, i);
        Array* func_irs = u->func_irs;

        // Bundle IR
        if (b->verbose > 2)
            printf("> Bundle IR: %s\n", u->path_ir);
        // IR Start
        str_clear(code);
        str_append_chars(code, u->ir_start);
        // IR vtable
        if(u->is_main)
            stage_5_vtable_ir(code, b);
        // Func IR
        for (int i = 0; i < func_irs->length; i++) {
            IRFuncIR* irf = array_get_index(func_irs, i);
            if(irf->func->is_used) {
                str_append_chars(code, irf->ir);
            }
        }
        // IR end
        str_append_chars(code, u->ir_end);

        // Hash
        char *ir_code = str_get_fake_chars(code);
        char *ir_hash = al(b->alc, 64);
        ctxhash(ir_code, ir_hash);

        char *old_hash = "";
        if (file_exists(u->path_cache)) {
            usize start = microtime();
            file_get_contents(hash_buf, u->path_cache);
            b->time_io += microtime() - start;
            old_hash = str_to_chars(b->alc, hash_buf);
        }
        if (!str_is(old_hash, ir_hash)) {
            usize start = microtime();
            u->ir_changed = true;
            u->hash = ir_hash;
            write_file(u->path_ir, ir_code, false);
            if (b->verbose > 2)
                printf("> IR changed: %s\n", u->path_ir);
            b->time_io += microtime() - start;
        }
    }
}

void stage_5_mark_used_rec(Func* func) {
    if(func->is_used)
        return;
    func->is_used = true;
    if(func->class && !func->class->is_used) {
        stage_5_mark_class_used(func->class);
    }

    Array* classes = func->used_classes;
    for(int i = 0; i < classes->length; i++) {
        Class* class = array_get_index(classes, i);
        stage_5_mark_class_used(class);
    }

    Array* funcs = func->used_functions;
    for(int i = 0; i < funcs->length; i++) {
        Func* func = array_get_index(funcs, i);
        stage_5_mark_used_rec(func);
    }
}

void stage_5_mark_class_used(Class* class) {
    if (class->is_used)
        return;
    class->is_used = true;
    Array *funcs = class->funcs->values;
    for (int i = 0; i < funcs->length; i++) {
        Func *cf = array_get_index(funcs, i);
        if (cf->use_if_class_is_used) {
            stage_5_mark_used_rec(cf);
        }
    }
}

void stage_5_vtable_ir(Str* code, Build* b) {

    // VTables
    int gc_vtables = b->gc_vtables;
    char gc_vt_count[20];
    itos((gc_vtables + 1) * 5, gc_vt_count, 10);
    char gc_vt_name_buf[256];
    Array *classes = b->classes;

    // Gen vtable
    str_preserve(code, 500);
    str_flat(code, "@valk_gc_vtable = unnamed_addr constant [");
    str_add(code, gc_vt_count);
    str_flat(code, " x ptr] [\n");
    str_flat(code, "ptr null, ptr null, ptr null, ptr null, ptr null"); // vtable start from index 1
    for (int i = 0; i < classes->length; i++) {
        Class *class = array_get_index(classes, i);
        if (class->type != ct_class)
            continue;

        Func *transfer = map_get(class->funcs, "_v_transfer");
        Func *mark = map_get(class->funcs, "_v_mark");
        Func *mark_shared = map_get(class->funcs, "_v_mark_shared");
        Func *share = map_get(class->funcs, "_v_share");
        Func *gc_free = map_get(class->funcs, "_gc_free");

        str_preserve(code, 1000);
        str_flat(code, ",\n");
        str_flat(code, "ptr ");
        str_add(code, ir_vtable_func_name(transfer, gc_vt_name_buf));
        str_flat(code, ", ptr ");
        str_add(code, ir_vtable_func_name(mark, gc_vt_name_buf));
        str_flat(code, ", ptr ");
        str_add(code, ir_vtable_func_name(mark_shared, gc_vt_name_buf));
        str_flat(code, ", ptr ");
        str_add(code, ir_vtable_func_name(share, gc_vt_name_buf));
        str_flat(code, ", ptr ");
        str_add(code, ir_vtable_func_name(gc_free, gc_vt_name_buf));
    }
    str_flat(code, "\n], align 8\n");
}

char* ir_vtable_func_name(Func* func, char* buf) {
    if(!func || !func->is_used)
        return "null";
    buf[0] = '@';
    buf[1] = '\0';
    strcat(buf, func->export_name);
    return buf;
}