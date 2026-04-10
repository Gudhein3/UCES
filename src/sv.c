#include "sv.h"
#include <string.h>

String_View sv_split_char(String_View *sv, char delim) {
    size_t i = 0;
    while (i < sv->size && sv->data[i] != delim) {
        i += 1;
    }
    String_View res = (String_View) {
        .data = sv->data,
        .size = i
    };
    if (sv->size-i == 0) --i;

    sv->size -= i+1;
    sv->data += i+1;
    return res;
}

String_View sv_split_group(String_View *sv, int(*is_delim)(int symbol)) {
    size_t i = 0;
    while (i < sv->size && !is_delim(sv->data[i])) {
        i += 1;
    }
    String_View res = (String_View) {
        .data = sv->data,
        .size = i
    };
    if (sv->size-i == 0) --i;

    sv->size -= i+1;
    sv->data += i+1;
    return res;
}

void sv_trim_left(String_View *sv) {
    while (sv->size > 0 && isspace(sv->data[0])) {
        sv->data += 1;
        sv->size -= 1;
    }
}

void sv_trim_right(String_View *sv) {
    while (sv->size > 0 && isspace(sv->data[sv->size-1])) {
        sv->size -= 1;
    }
}

int sv_cmp_cstr(String_View sv, const char *cstr) {
    if (sv.size == 0) return 0-cstr[0];
    size_t n = strlen(cstr);
    if (sv.size < n) {
        return 0-cstr[sv.size-1];
    }
    if (n < sv.size) {
        return sv.data[n-1]-0;
    }
    return strncmp(sv.data, cstr, sv.size);
}

int sv_cmp_sv(String_View a, String_View b) {
    if (a.size < b.size) {
        return 0-b.data[a.size-1];
    }
    if (b.size < a.size) {
        return a.data[b.size-1]-0;
    }
    return strncmp(a.data, b.data, a.size);
}
