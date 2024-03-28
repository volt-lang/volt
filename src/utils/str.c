
#include "../all.h"

Str *str_make(Allocator *alc, int mem_size) {
    if (mem_size == 0) {
        mem_size = 20;
    }
    Str *result = al(alc, sizeof(Str));
    result->alc = alc;
    result->length = 0;
    result->mem_size = mem_size;
    result->data = al(alc, mem_size);
    return result;
}

char* str_get_fake_chars(Str* str) {
    char* res = str->data;
    res[str->length] = 0;
    return res;
}

void str_append(Str *str, Str *add) {
    if (add->length == 0) {
        return;
    }
    int new_length = str->length + add->length;
    bool reloc = str->mem_size < new_length;
    while (str->mem_size < new_length) {
        str->mem_size *= 2;
    }
    if (reloc) {
        void *prev = str->data;
        str->data = al(str->alc, str->mem_size);
        memcpy(str->data, prev, str->length);
    }
    memcpy(str->data + str->length, add->data, add->length);
    str->length = new_length;
}

void str_append_char(Str *str, char ch) {
    int new_length = str->length + 1;
    bool reloc = str->mem_size < new_length;
    while (str->mem_size < new_length) {
        str->mem_size *= 2;
    }
    if (reloc) {
        void *prev = str->data;
        str->data = al(str->alc, str->mem_size);
        memcpy(str->data, prev, str->length);
    }
    char *adr = str->data + str->length;
    *adr = ch;
    str->length = new_length;
}

void str_append_chars(Str *str, char *add) {
    int add_len = strlen(add);
    str_append_from_ptr(str, add, add_len);
}

void str_increase_memsize(Str *str, int new_memsize) {
    void* data = al(str->alc, new_memsize);
    memcpy(data, str->data, str->length);
    str->data = data;
    str->mem_size = new_memsize;
}
void str_preserve(Str *str, int size) {
    int space_left = str->mem_size - str->length;
    if(space_left < size) {
        int new_size = str->mem_size * 2;
        char* data = al(str->alc, new_size);
        memcpy(data, str->data, str->length);
        str->data = data;
        str->mem_size = new_size;
    }
}

void str_append_from_ptr(Str *str, void *ptr, int len) {
    //
    if (len == 0) {
        return;
    }
    int new_length = str->length + len;
    bool reloc = str->mem_size < new_length;
    while (str->mem_size < new_length) {
        str->mem_size *= 2;
    }
    if (reloc) {
        void *prev = str->data;
        str->data = al(str->alc, str->mem_size);
        memcpy(str->data, prev, str->length);
    }
    memcpy(str->data + str->length, ptr, len);
    str->length = new_length;
}

void str_add_x(Str *str, char *add, int len) {
    // if (str->mem_size < str->length + len) {
    //     printf("NOT ENOUGH MEM (1) '%s'\n", add);
    // }
    memcpy(str->data + str->length, add, len);
    str->length += len;
}
void str_add(Str *str, char *add) {
    int len = strlen(add);
    // if (str->mem_size < str->length + len) {
    //     printf("NOT ENOUGH MEM (2) '%s'\n", add);
    // }
    memcpy(str->data + str->length, add, len);
    str->length += len;
}

char *str_to_chars(Allocator *alc, Str *str) {
    char *res = alc ? al(alc, str->length + 1) : malloc(str->length + 1);
    memcpy(res, str->data, str->length);
    res[str->length] = '\0';
    return res;
}

void str_clear(Str *str) {
    str->length = 0;
}

char* str_temp_chars(Str *str) {
    ((char*)(str->data))[str->length] = '\0';
    return str->data;
}
