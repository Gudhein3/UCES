#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _Nullable

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef  int8_t  s8;
typedef  int16_t s16;
typedef  int32_t s32;
typedef  int64_t s64;

typedef float f32;
typedef double f64;

#define __STR0(x) #x
#define __STR(x) __STR0(x)

typedef int(*dev_func_poke_t)(u32 addr, u8 value);
typedef int(*dev_func_peek_t)(u32 addr, u8 *result);

typedef struct {
    u8 *memory;
    u32 memory_size;
    u32 registers[30];
    u32 pc;
} VM;
extern VM vm;

typedef struct {
    dev_func_poke_t poke;
    dev_func_peek_t peek;
} Dev;

typedef enum {
    OP_ADD   = 0x00,
    OP_SUB   = 0x01,
    OP_AND   = 0x02,
    OP_OR    = 0x03,
    OP_XOR   = 0x04,
    OP_NAND  = 0x05,
    OP_NOR   = 0x06,
    OP_XNOR  = 0x07,
    OP_NEG   = 0x08,
    OP_NOT   = 0x09,
    OP_DIV   = 0x0A,
    OP_IDIV  = 0x0B,
    OP_REM   = 0x0C,
    OP_IREM  = 0x0D,
    OP_MUL   = 0x0E,
    OP_IMUL  = 0x0F,

    OP_WR8   = 0x10,
    OP_WR16  = 0x11,
    OP_WR32  = 0x12,

    OP_RD8   = 0x13,
    OP_RD16  = 0x14,
    OP_RD32  = 0x15,

    OP_RDS8  = 0x16,
    OP_RDS16 = 0x17,

    OP_CMP   = 0x18,
    OP_MV    = 0x19,
    OP_MVE   = 0x20,
    OP_MVO   = 0x21,

    OP_TSBI  = 0x22,
    OP_TSI   = 0x23,
    OP_TSB   = 0x24,
    OP_TS    = 0x25,

    OP_CALL  = 0x26,
    OP_RET   = 0x27,
    OP_PUSH  = 0x28,
    OP_POP   = 0x29,

    OP_LSL   = 0x2A,
    OP_LSR   = 0x2B,

    OP_ANDI  = 0xEA,
    OP_ORI   = 0xEB,
    OP_WAIT  = 0xEC,
    OP_ADDI  = 0xED,
    OP_HLT   = 0xEE,
    OP_UDI   = 0xEF,

    OP_LDLX  = 0xFC,
    OP_LDHX  = 0xFD,
    OP_LDL   = 0xFE,
    OP_LDH   = 0xFF,
} OpCode;
