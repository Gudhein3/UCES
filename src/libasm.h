#pragma once
#include "uces.h"
#include "sv.h"

#define INST_NOARG (1<<0)
#define INST_NOOUT (1<<1)
#define INST_STND (INST_NOARG|INST_NOOUT)

#define INST_1IMM (1<<2)
#define INST_2IMM (1<<3)
// No second argument
#define INST_NO2A (1<<4)

#define INST_SING (INST_NOOUT|INST_NO2A)

typedef struct {
    OpCode op;
    u16 flags;
    const char *name;
} AsmInst;

typedef struct {
    u8 *items;
    size_t count;
    size_t capacity;
} ByteArray;

typedef struct {
    const u8 *data;
    size_t size;
} Byte_View;

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} String_Builder;

typedef enum {
    ASM_SYMBOL_LOCAL = 0,
    ASM_SYMBOL_GLOBAL = 1,
} SymbolType;

typedef struct {
    SymbolType type;
    String_View label; // Just slice it from a source code.
    u32 address;
} Symbol;

typedef struct {
    Symbol *items;
    size_t count, capacity;
} SymbolTable;

#define DA_INITIAL_CAPACITY 256

#define da_append(xs, x)                                                             \
    do {                                                                             \
        if ((xs)->count >= (xs)->capacity) {                                         \
            if ((xs)->capacity == 0) (xs)->capacity = DA_INITIAL_CAPACITY;           \
            else (xs)->capacity += (xs)->capacity>>1;                                \
            (xs)->items = realloc((xs)->items, (xs)->capacity*sizeof(*(xs)->items)); \
        }                                                                            \
        (xs)->items[(xs)->count++] = (x);                                            \
    } while (0)

#define da_extend(xs, xc, x)                                                         \
    do {                                                                             \
        if ((xs)->count+(xc) >= (xs)->capacity) {                                    \
            if ((xs)->capacity == 0) (xs)->capacity = DA_INITIAL_CAPACITY;           \
            else (xs)->capacity += ((xs)->capacity>>1) + (xc);                       \
            (xs)->items = realloc((xs)->items, (xs)->capacity*sizeof(*(xs)->items)); \
        }                                                                            \
        memcpy((xs)->items+(xs)->count, (x), xc);                                    \
        (xs)->count += (xc);                                                         \
    } while (0)

extern AsmInst asm_instructions[]; // Terminated with (AsmInst) {0, 0, NULL}
int asm_export_symbols(String_View source_code, SymbolTable *table);
int assemble(String_View source_code, ByteArray *output, SymbolTable _Nullable *symbols_table);
int unassemble(Byte_View bin, String_Builder *sb);
