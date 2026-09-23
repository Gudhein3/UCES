#include "cpu.h"
#include "libasm.h"

static int is_pc_rewritten = 0;

u32 read_register(u8 r) {
    if (r == 0) return 0;
    if (r == 1) return vm.pc;
    if (r >= 32) return 0;
    return vm.registers[r-2];
}

void write_register(u8 r, u32 d) {
    if (r == 0) return;
    if (r == 1) {
        vm.pc = d;
        is_pc_rewritten = 1;
        return;
    }
    if (r >= 32) return;
    vm.registers[r-2] = d;
}

void write_registerl(u8 r, u16 d) {
    if (r == 0) return;
    if (r == 1) {
        vm.pc = vm.pc&0xFFFF0000|d;
        is_pc_rewritten = 1;
        return;
    }
    if (r >= 32) return;
    vm.registers[r-2] = vm.registers[r-2]&0xFFFF0000|d;
}

void write_registerh(u8 r, u16 d) {
    if (r == 0) return;
    if (r == 1) {
        vm.pc = vm.pc&0x0000FFFF|(d<<16);
        is_pc_rewritten = 1;
        return;
    }
    if (r >= 32) return;
    vm.registers[r-2] = vm.registers[r-2]&0x0000FFFF|(d<<16);
}

#define RR read_register
#define WR write_register
#define WRl write_registerl
#define WRh write_registerh

void dump() {
    printf("---- DUMP ----\n");
    printf("Registers:\n");
    for (int i = 0; i < 32; ++i) {
        printf("x%-2d %-4s: 0x%08X\n", i, regnames[i], RR(i));
    }
    printf("VM state:\n");
    printf("memory: %p\n", vm.memory);
    printf("memory_size: 0x%08X\n", vm.memory_size);
}

#define MW8 POKE8
#define MW16 POKE16
#define MW32 POKE32

#define MR8 PEEK8
#define MR16 PEEK16
#define MR32 PEEK32

#define MRS8 PEEKS8
#define MRS16 PEEKS16

void fetch_instruction(u32 pc, OpCode *op, u8 *ar, u8 *br, u8 *cr, u16 *imm) {
    *op = PEEK8(pc);
    *ar = PEEK8(pc+1);
    *br = PEEK8(pc+2);
    *cr = PEEK8(pc+3);
    *imm = ((u16)*br<<8)|(u16)*ar;
}

typedef EIS(*CpuInstImpl)(u8 ar, u8 br, u8 cr, u32 a, u32 b, u16 imm);

#define define_instruction(name) static EIS cpu_##name(u8 ar, u8 br, u8 cr, u32 a, u32 b, u16 imm)

#include "insts.c.in"

static CpuInstImpl procedure_array[256] = {
    [OP_ADD]   = cpu_add,
    [OP_SUB]   = cpu_sub,
    [OP_AND]   = cpu_and,
    [OP_OR]    = cpu_or,
    [OP_XOR]   = cpu_xor,
    [OP_NAND]  = cpu_nand,
    [OP_NOR]   = cpu_nor,
    [OP_XNOR]  = cpu_xnor,
    [OP_NEG]   = cpu_neg,
    [OP_NOT]   = cpu_not,
    [OP_DIV]   = cpu_div,
    [OP_IDIV]  = cpu_idiv,
    [OP_REM]   = cpu_rem,
    [OP_IREM]  = cpu_irem,
    [OP_MUL]   = cpu_mul,
    [OP_IMUL]  = cpu_imul,
    [OP_WR8]   = cpu_wr8,
    [OP_WR16]  = cpu_wr16,
    [OP_WR32]  = cpu_wr32,
    [OP_RD8]   = cpu_rd8,
    [OP_RD16]  = cpu_rd16,
    [OP_RD32]  = cpu_rd32,
    [OP_RDS8]  = cpu_rds8,
    [OP_RDS16] = cpu_rds16,
    [OP_CMP]   = cpu_cmp,
    [OP_MV]    = cpu_mv,
    [OP_MVE]   = cpu_mve,
    [OP_MVO]   = cpu_mvo,
    [OP_TSBI]  = cpu_tsbi,
    [OP_TSI]   = cpu_tsi,
    [OP_TSB]   = cpu_tsb,
    [OP_TS]    = cpu_ts,
    [OP_CALL]  = cpu_call,
    [OP_RET]   = cpu_ret,
    [OP_PUSH]  = cpu_push,
    [OP_POP]   = cpu_pop,
    [OP_LSL]   = cpu_lsl,
    [OP_LSR]   = cpu_lsr,
    [OP_ANDI]  = cpu_andi,
    [OP_ORI]   = cpu_ori,
    [OP_DEBUG] = cpu_debug,
    [OP_ADDI]  = cpu_addi,
    [OP_HLT]   = cpu_hlt,
    [OP_LDLX]  = cpu_ldlx,
    [OP_LDHX]  = cpu_ldhx,
    [OP_LDL]   = cpu_ldl,
    [OP_LDH]   = cpu_ldh
};

int cpu_tick() {
    is_pc_rewritten = 0;
    OpCode op;
    u8 ar, br, cr;
    u32 a, b;
    u16 imm;
    fetch_instruction(vm.pc, &op, &ar, &br, &cr, &imm);

    a = RR(ar);
    b = RR(br);
    u32 orig_pc = vm.pc;

    CpuInstImpl procedure = procedure_array[op & 255];
    if (procedure) {
        EIS res = procedure(ar, br, cr, a, b, imm);
        if (res != 0) return res;
    }
    else {
        fault("Illegal instruction: 0x%02X at 0x%08X", op, vm.pc);
    }
    if (is_pc_rewritten == 1) {
        if (DBG >= 2)
            fprintf(stderr, "[DEBUG] Jump %08X -> %08X\n", orig_pc, RR(001));
        return EIS_OVPC;
    }
    return 0;
}
