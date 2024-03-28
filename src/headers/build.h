
#ifndef _H_BUILD
#define _H_BUILD

#include "typedefs.h"

int cmd_build(int argc, char *argv[]);
Str* build_get_str_buf(Build* b);
void build_return_str_buf(Build* b, Str* buf);
Map* build_cc_defs(Allocator* alc, Map* options);

Pkc *pkc_make(Allocator *alc, Build *b, char *name_suggestion);
void pkc_set_dir(Pkc *pkc, char *dir);
Pkc *pkc_load_pkc(Pkc *pkc, char *name, Parser* p);
Fc* pkc_load_header(Pkc* pkc, char* fn, Parser* p, bool is_sub_header);

Nsc *nsc_make(Allocator *alc, Pkc *pkc, char *name, char *dir);
Nsc *nsc_load(Pkc *pkc, char *name, bool must_exist, Parser* p);
Nsc *get_valk_nsc(Build *b, char *name);

Fc *fc_make(Nsc *nsc, char *path, bool is_sub_header);

// Stage functions
void build_set_stages(Build *b);
void stage_add_item(Stage *stage, void *item);
void build_run_stages(Build *b);
// Stages
void stage_1_parse(Fc *fc);
void stage_2_alias(Unit *u);
void stage_2_props(Unit *u);
void stage_2_update_classes(Build* b);
void stage_2_types(Unit *u);
void stage_3_values(Unit *u);
void stage_3_gen(Build* b);
void stage_4_ast(Unit *u);
void stage_4_ir(Unit *u);
void stage_4_ast_main(Unit *u);
void stage_5_ir_final(Build* b);
void stage_5_objects(Build *b);
void stage_6_link(Build* b, Array* o_files);
// Sub stages
void stage_props_class(Parser* p, Class *class, bool is_trait);
void stage_types_func(Parser* p, Func *func);
void stage_types_class(Parser* p, Class* class);
void read_ast(Parser *p, bool single_line);
//

struct Build {
    Allocator *alc;
    Allocator *alc_ast;
    //
    char *cache_dir;
    char *path_out;
    //
    char *char_buf;
    Str *str_buf;
    usize time_lex;
    usize time_parse;
    usize time_io;
    usize time_ir;
    usize time_llvm;
    usize time_link;
    Stage *stage_1_parse;
    Stage *stage_2_alias;
    Stage *stage_2_props;
    Stage *stage_2_class_sizes;
    Stage *stage_2_types;
    Stage *stage_3_values;
    Stage *stage_3_gen;
    Stage *stage_4_ast;
    //
    Map *pkc_by_dir;
    Map *fc_by_path;
    Map *nsc_by_path;
    Array *used_pkc_names;
    Array *pkcs;
    //
    Pkc *pkc_main;
    Pkc *pkc_valk;
    Nsc *nsc_main;
    //
    Func *func_main;
    Func *func_main_gen;
    Func *func_mark_globals;
    Func *func_mark_shared;
    //
    Array *units;
    Array *classes;
    Array *pool_str;
    ErrorCollection* errors;
    Array *strings;
    Array *links;
    Map *link_settings;
    //
    Map* cc_defs;
    //
    size_t mem_parse;
    size_t mem_objects;
    //
    int host_os;
    int target_os;
    int host_arch;
    int target_arch;
    //
    int ptr_size;
    int error_count;
    int export_count;
    int string_count;
    int gc_vtables;
    int verbose;
    int LOC;
    //
    bool parser_started;
    bool is_test;
    bool is_clean;
    bool optimize;
};
struct Fc {
    Build *b;
    char *path;
    //
    Allocator *alc;
    Nsc *nsc;
    Pkc *header_pkc;
    Scope *scope;
    //
    Chunk *content;
    //
    bool is_header;
};
struct Nsc {
    Pkc *pkc;
    char *name;
    char *dir;
    Scope *scope;
    Unit *unit;
};
struct Pkc {
    Build *b;
    char *dir;
    char *name;
    PkgConfig *config;
    Map *namespaces;
    Map *pkc_by_name;
    Array *header_dirs;
    Map *headers_by_fn;
};
struct Stage {
    Array *items;
    void (*func)(void *);
    int i;
};
struct CompileData {
    Build* b;
    Array* ir_files;
    char* path_o;
    //
    void* target_machine;
    void* target_data;
    char* triple;
    char* data_layout;
};
struct Link {
    char* name;
    int type;
};

#endif