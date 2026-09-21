#pragma once
#include "uces.h"
#include "da.h"
#include "cpu.h"
#include "strings.h"

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

extern const char *regnames[];
extern AsmInst asm_instructions[]; // Terminated with (AsmInst) {0, 0, NULL}
int asm_export_symbols(String_View source_code, SymbolTable *table);
int assemble(String_View source_code, ByteArray *output, SymbolTable _Nullable *symbols_table);
int unassemble(Byte_View bin, String_Builder *sb);
