
#ifndef _H_STR
#define _H_STR

#include "typedefs.h"

struct Str {
    Allocator *alc;
    int length;
    int mem_size;
    void *data;
};

Str *str_make(Allocator *alc, int mem_size);
char* str_get_fake_chars(Str* str);
void str_append(Str *, Str *);
void str_append_char(Str *, char);
void str_append_chars(Str *, char *);
void str_append_from_ptr(Str *str, void *ptr, int len);
void str_add_x(Str *str, char *add, int len);
void str_add(Str *str, char *add);
char *str_to_chars(Allocator *alc, Str *);
void str_clear(Str *str);
void str_increase_memsize(Str *str, int new_memsize);
void str_preserve(Str *str, int size);
char* str_temp_chars(Str *str);

#endif
