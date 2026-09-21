#include <stdbool.h>
#include "uces.h"

u8 *read_file(const char *filename, size_t *_Nullable size) {
    FILE *fptr = fopen(filename, "rb");
    if (!fptr) return NULL;
    size_t _size;
    fseek(fptr, 0, SEEK_END);
    _size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    u8 *buf = malloc(_size);
    if (!buf) {
        fclose(fptr);
        return NULL;
    }
    fread(buf, _size, 1, fptr);
    fclose(fptr);
    if (size) *size = _size;
    return buf;
}
