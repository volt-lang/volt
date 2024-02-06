
#include "../all.h"

void stage_values(Fc *fc);

void stage_3_values(Fc* fc) {
    Build* b = fc->b;

    if (b->verbose > 2)
        printf("Stage 3 | Scan values: %s\n", fc->path);

    usize start = microtime();
    stage_values(fc);
    b->time_parse += microtime() - start;

    stage_add_item(b->stage_4_ast, fc);
}

void stage_values(Fc *fc) {
    Build *b = fc->b;

    Array* globals = fc->globals;
    for(int i = 0; i < globals->length; i++) {
        Global* g = array_get_index(globals, i);

        if(!g->chunk_value) {
            if(!g->type->nullable) {
                sprintf(b->char_buf, "Globals with a non-null type require a default value");
                parse_err(fc->chunk_parse, b->char_buf);
            }
            g->value = value_make(b->alc, v_null, NULL, g->type);
            continue;
        }

        *fc->chunk_parse = *g->chunk_value;
        Type *type = read_type(fc, fc->alc, fc->scope, false);
        g->type = type;
    }
}
