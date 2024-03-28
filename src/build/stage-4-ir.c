
#include "../all.h"

void stage_4_ir(Unit* u) {

    Build* b = u->b;

    // printf("💾 Before: %.2f MB\n", (double)(get_mem_usage()) / (1024 * 1024));

    if (b->verbose > 2)
        printf("Stage 4 | Generate IR: %s\n", u->nsc->name);

    usize start = microtime();
    IR* ir = ir_make(u, u->parser);
    b->time_ir += microtime() - start;

    Allocator* alc = u->b->alc_ast;
    alc_wipe(alc);
}
