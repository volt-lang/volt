
#include "../all.h"

void unit_validate(Unit *u, Parser *p) {

    Build* b = u->b;
    Allocator* alc = b->alc; 

    Array* classes = u->classes;
    int class_count = classes->length;
    for (int i = 0; i < class_count; i++) {
        Class *class = array_get_index(classes, i);
        validate_class(p, class);
    }

    if(u->is_main) {
        Func *main = b->func_main;
        if (main) {
            func_validate_arg_count(p, main, false, 0, 1);

            if(main->arg_types->length > 0) {
                Array *types = array_make(b->alc, 2);
                Class *class_array = get_valk_class(b, "type", "Array");
                Class *gclass = get_generic_class(p, class_array, gen_type_array_1(alc, b, "String", false));
                array_push(types, type_gen_class(b->alc, gclass));

                func_validate_arg_type(p, main, 0, types);
            }

            func_validate_rett(p, main, gen_type_array_2(alc, b, NULL, false, "i32", false));
        }
    }
}

void validate_class(Parser *p, Class* class) {

    Build *b = p->b;
    Allocator* alc = b->alc; 

    Map *funcs = class->funcs;

    Func *add = map_get(funcs, "_add");
    if (add)
        func_validate_arg_count(p, add, false, 2, 2);

    Func *string = map_get(funcs, "_string");
    if (string) {
        func_validate_arg_count(p, string, false, 1, 1);
        func_validate_rett(p, string, gen_type_array_1(alc, b, "String", false));
    }

    Func *gc_free = map_get(funcs, "_gc_free");
    if (gc_free) {
        gc_free->use_if_class_is_used = true;
        func_validate_arg_count(p, gc_free, false, 1, 1);
        func_validate_rett_void(p, gc_free);
    }

    Func *gc_transfer = map_get(funcs, "_gc_transfer");
    if (gc_transfer) {
        func_validate_arg_count(p, gc_transfer, false, 1, 1);
        func_validate_rett_void(p, gc_transfer);
    }

    Func *gc_mark = map_get(funcs, "_gc_mark");
    if (gc_mark) {
        func_validate_arg_count(p, gc_mark, false, 1, 1);
        func_validate_rett_void(p, gc_mark);
    }

    Func *gc_mark_shared = map_get(funcs, "_gc_mark_shared");
    if (gc_mark_shared) {
        func_validate_arg_count(p, gc_mark_shared, false, 1, 1);
        func_validate_rett_void(p, gc_mark_shared);
    }

    Func *gc_share = map_get(funcs, "_gc_share");
    if (gc_share) {
        func_validate_arg_count(p, gc_share, false, 1, 1);
        func_validate_rett_void(p, gc_share);
    }

    Func *each = map_get(funcs, "_each");
    if (each) {
        func_validate_arg_count(p, each, false, 2, 2);
        func_validate_rett_count(p, each, false, 2, 2);
    }
}