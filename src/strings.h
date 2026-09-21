#pragma once
#include <stddef.h>
#include <ctype.h>

typedef struct {
    const char *data;
    size_t size;
} String_View;

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} String_Builder;

String_View sv_split_char(String_View *sv, char delim);
String_View sv_split_group(String_View *sv, int(*is_delim)(int symbol));
void sv_trim_left(String_View *sv);
void sv_trim_right(String_View *sv);
int sv_cmp_cstr(String_View sv, const char *cstr);
int sv_cmp_sv(String_View a, String_View b);

// TODO: Add note for the compiler that this function behaves like printf.
void sb_printf(String_Builder *sb, const char *fmt, ...);
