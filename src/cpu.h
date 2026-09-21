#ifndef _CPU_INCLUDE
#define _CPU_INCLUDE
#include "uces.h"
#include "mmu.h"

typedef enum {
    EIS_HALT = 1<<0,
    EIS_OVPC = 1<<1,
} EIS;

typedef struct {
    u8 *memory;
    u32 memory_size;
    u32 registers[30];
    u32 pc;
} VM;
extern VM vm;

void dump();
#define fault(fmt, ...) do {dump(); fprintf(stderr, "fault " __FILE__ "," __STR(__LINE__) ": " fmt"\n", __VA_ARGS__); abort();} while(0)
#define panic(fmt, ...) do {fprintf(stderr, "panic " __FILE__ "," __STR(__LINE__) ": " fmt"\n", __VA_ARGS__); abort();} while(0)

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
    OP_DEBUG = 0xEC,
    OP_ADDI  = 0xED,
    OP_HLT   = 0xEE,
    OP_UDI   = 0xEF,
    OP_LDLX  = 0xFC,
    OP_LDHX  = 0xFD,
    OP_LDL   = 0xFE,
    OP_LDH   = 0xFF,
} OpCode;

u32 read_register(u8 r);
void write_register(u8 r, u32 d);
void write_registerl(u8 r, u16 d);
void write_registerh(u8 r, u16 d);

// op - the instruction's opcode
// ar, br, cr - ids of first register, second register, third register, correspondingly
// imm - ((u16)br<<8)|(u16)ar
void fetch_instruction(u32 pc, OpCode *op, u8 *ar, u8 *br, u8 *cr, u16 *imm);
int cpu_tick();


#endif // _CPU_INCLUDE
