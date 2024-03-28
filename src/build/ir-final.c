
#include "../all.h"

void ir_gen_final(IR* ir) {
    //
    Unit* u = ir->unit;
    Str* code = ir->code_final;

    // Structs & Globals
    str_append(code, ir->code_struct);
    str_flat(code, "\n");
    str_append(code, ir->code_global);
    str_flat(code, "\n\n");

    u->ir_start = str_to_chars(u->b->alc, code);
    str_clear(code);

    // Functions
    for (int i = 0; i < ir->funcs->length; i++) {
        IRFunc *func = array_get_index(ir->funcs, i);
        ir_func_definition(code, ir, func->func, false, func->rett_refs);
        // Blocks
        if(func->block_code->code->length > 0) {
            for (int o = 0; o < func->blocks->length; o++) {
                IRBlock *block = array_get_index(func->blocks, o);
                str_preserve(code, block->code->length + 100);
                str_add(code, block->name);
                str_flat(code, ":\n");
                str_append(code, block->code);
            }
        }
        //
        str_flat(code, "}\n\n");

        IRFuncIR* irf = al(u->b->alc, sizeof(IRFuncIR));
        irf->func = func->func;
        irf->ir = str_to_chars(u->b->alc, code);
        array_push(u->func_irs, irf);
        str_clear(code);
    }

    // Extern
    str_append(code, ir->code_extern);
    str_flat(code, "\n");

    // Attrs
    for (int i = 0; i < ir->attrs->length; i++) {
        str_add(code, array_get_index(ir->attrs, i));
        str_flat(code, "\n");
    }
    str_append(code, ir->code_attr);
    str_flat(code, "\n");

    u->ir_end = str_to_chars(u->b->alc, code);
    str_clear(code);
}
