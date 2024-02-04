
#include "../all.h"

Class* class_make(Allocator* alc, Build* b) {
    Class* c = al(alc, sizeof(Class));
    c->b = b;
    c->name = NULL;
    c->body = NULL;
    c->size = 0;
    c->props = map_make(alc);
    c->funcs = map_make(alc);
    return c;
}
