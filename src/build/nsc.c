
#include "../all.h"

Nsc* nsc_make(Allocator* alc, Pkc* pkc, char* name, char* dir) {
    Nsc* nsc = al(alc, sizeof(Nsc));
    nsc->pkc = pkc;
    nsc->name = name;
    nsc->dir = dir;
    nsc->scope = scope_make(alc, sc_default, NULL);

    char *path_o = al(alc, VOLT_PATH_MAX);
    sprintf(path_o, "%s%s_%s.o", pkc->b->cache_dir, nsc->name, nsc->pkc->name);
    char *path_ir = al(alc, VOLT_PATH_MAX);
    sprintf(path_ir, "%s%s_%s.ir", pkc->b->cache_dir, nsc->name, nsc->pkc->name);
    char *path_cache = al(alc, VOLT_PATH_MAX);
    sprintf(path_ir, "%s%s_%s.json", pkc->b->cache_dir, nsc->name, nsc->pkc->name);

    Unit* u = al(alc, sizeof(Unit));
    u->b = pkc->b;
    u->nsc = nsc;
    //
    u->path_o = path_o;
    u->path_ir = path_ir;
    u->path_cache = path_cache;
    u->hash = NULL;
    //
    u->funcs = array_make(alc, 50);
    u->classes = array_make(alc, 50);
    u->aliasses = array_make(alc, 20);
    u->globals = array_make(alc, 20);

    u->ir_changed = false;

    nsc->unit = u;

    array_push(pkc->b->units, u);

    return nsc;
}

Nsc* nsc_load(Pkc* pkc, char* name, bool must_exist, Parser* p) {
    Nsc* nsc = map_get(pkc->namespaces, name);
    if(nsc)
        return nsc;

    // Check config
    Build* b = pkc->b;
    if(!pkc->config) {
        if(!must_exist)
            return NULL;
        p ? parse_err(p, -1, "Trying to load namespace '%s' from a package without a config", name)
        : build_err(b, "Trying to load namespace '%s' from a package without a config", name);
    }

    char* dir = cfg_get_nsc_dir(pkc->config, name, b->alc);
    if(!dir) {
        if(!must_exist)
            return NULL;
        p ? parse_err(p, -1, "Namespace '%s' not found in config: '%s'", name, pkc->config->path)
        : build_err(b, "Namespace '%s' not found in config: '%s'", name, pkc->config->path);
    }
    if(!file_exists(dir)) {
        if(!must_exist)
            return NULL;
        p ? parse_err(p, -1, "Namespace directory of '%s' does not exist: '%s'", name, dir)
        : build_err(b, "Namespace directory of '%s' does not exist: '%s'", name, dir);
    }

    Nsc* ns2 = map_get(b->nsc_by_path, dir);
    if(ns2) {
        p ? parse_err(p, -1, "There are 2 namesapces pointing to the same directory: '%s' | '%s' => '%s' ", ns2->name, name, dir)
        : build_err(b, "There are 2 namesapces pointing to the same directory: '%s' | '%s' => '%s' ", ns2->name, name, dir);
    }

    nsc = nsc_make(b->alc, pkc, name, dir);

    Array* files = get_subfiles(b->alc, dir, false, true);
    for(int i = 0; i < files->length; i++) {
        char* path = array_get_index(files, i);
        if(ends_with(path, ".vo"))
            fc_make(nsc, path);
    }

    map_set_force_new(pkc->namespaces, name, nsc);
    return nsc;
}


Nsc* get_volt_nsc(Build* b, char* name) {
    //
    Pkc* pkc = b->pkc_volt;
    if(!pkc)
        return NULL;
    return nsc_load(pkc, name, true, NULL);
}
