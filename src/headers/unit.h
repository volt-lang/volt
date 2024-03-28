
#ifndef _H_UNIT
#define _H_UNIT

#include "typedefs.h"

void unit_validate(Unit *u, Parser *p);
void validate_class(Parser *p, Class* class);

struct Unit {
    Build *b;
    Nsc *nsc;
    //
    char *path_o;
    char *path_ir;
    char *path_cache;
    char *hash;
    char *unique_hash;
    //
    char *ir_start;
    char *ir_end;
    Array *func_irs;
    //
    Parser *parser;
    //
    Array *funcs;
    Array *classes;
    Array *aliasses;
    Array *globals;
    Array *tests;
    // Pools
    Array *pool_parsers;
    //
    int id;
    int string_count;
    int export_count;
    //
    bool ir_changed;
    bool is_main;
};

#endif