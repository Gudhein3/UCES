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

    switch (op) {
    case OP_ADD: WR(cr, a+b);
        break;
    case OP_SUB: WR(cr, a-b);
        break;
    case OP_AND: WR(cr, a&b);
        break;
    case OP_OR: WR(cr, a|b);
        break;
    case OP_XOR: WR(cr, a^b);
        break;
    case OP_NAND: WR(cr, ~(a&b));
        break;
    case OP_NOR: WR(cr, ~(a|b));
        break;
    case OP_XNOR: WR(cr, ~(a^b));
        break;
    case OP_NEG: WR(cr, -a);
        break;
    case OP_NOT: WR(cr, ~a);
        break;
    case OP_DIV: WR(cr, (u32)a/(u32)b);
        break;
    case OP_IDIV: WR(cr, (s32)a/(s32)b);
        break;
    case OP_REM: WR(cr, (u32)a%(u32)b);
        break;
    case OP_IREM: WR(cr, (s32)a%(s32)b);
        break;
    case OP_MUL: WR(cr, (u32)a*(u32)b);
        break;
    case OP_IMUL: WR(cr, (s32)a*(s32)b);
        break;
    case OP_WR8: MW8(a, b);
        break;
    case OP_WR16: MW16(a, b);
        break;
    case OP_WR32: MW32(a, b);
        break;
    case OP_RD8: WR(cr, MR8(a));
        break;
    case OP_RD16: WR(cr, MR16(a));
        break;
    case OP_RD32: WR(cr, MR32(a));
        break;
    case OP_RDS8: WR(cr, MRS8(a));
        break;
    case OP_RDS16: WR(cr, MRS16(a));
        break;
    case OP_CMP: WR(cr, (
                         ((a!=0 && (a*b)/a!=b)  << 10) | // Overflow
                         (((a + b) < a)         << 9)  | // Carry
                         (((a - b) < a)         << 8)  | // Carry
                         ((a >  b)              << 7)  |
                         ((a <  b)              << 6)  |
                         (((s32)a > (s32)b)     << 5)  |
                         (((s32)a < (s32)b)     << 4)  |
                         ((a != b)              << 3)  |
                         ((a == b)              << 2)  |
                         ((b == 0)              << 1)  |
                         ((a == 0)              << 0)));
        break;
    case OP_MV: WR(cr, a);
        break;
    case OP_MVE: if (a != 0)            WR(cr, b);
        break;
    case OP_MVO: if (a == 0)            WR(cr, b);
        break;
    case OP_TSBI: WR(cr, (a & (1<<br)) != 0);
        break;
    case OP_TSI:  WR(cr, (a & br     ) != 0);
        break;
    case OP_TSB:  WR(cr, (a & (1<<b) ) != 0);
        break;
    case OP_TS:   WR(cr, (a & b      ) != 0);
        break;
        // NOTE: Register 003 is the stack pointer and 001 is the program counter.
    case OP_CALL: MW32(RR(003), vm.pc+4);WR(003, RR(003)+4);WR(001, a);
        break;
    case OP_RET:  WR(003, RR(003)-4);WR(001, MR32(RR(003)));
        break;
    case OP_PUSH: MW32(RR(003), a);WR(003, RR(003)+4);
        break;
    case OP_POP:  WR(003, RR(003)-4);WR(cr, MR32(RR(003)));
        break;
    case OP_LSL:  WR(cr, (u32)a<<(u32)b);
        break;
    case OP_LSR:  WR(cr, (u32)a>>(u32)b);
        break;
    case OP_ANDI: WR(cr, a&br);
        break;
    case OP_ORI: WR(cr, a|br);
        break;
    case OP_DEBUG:
        if (DBG >= 1) {
            FILE *fptr = fopen("memory-dump.bin", "wb");
            fwrite(vm.memory, 1, vm.memory_size, fptr);
            fclose(fptr);
            dump();
            printf("Press enter to continue: ");
            fflush(stdout);
            getchar();
        }
        break;
    case OP_HLT:
        return EIS_HALT;
    case OP_ADDI: WR(cr, a+(int8_t)br);
        break;
    case OP_LDLX: WR(cr, imm);
        break;
    case OP_LDHX: WR(cr, imm<<16);
        break;
    case OP_LDL:
        WRl(cr, imm);
        break;
    case OP_LDH:
        WRh(cr, imm);
        break;
    case OP_UDI:
    default:
        fault("Illegal instruction: 0x%02X at 0x%08X", op, vm.pc);
    }
    if (is_pc_rewritten == 1) {
        if (DBG >= 2)
            fprintf(stderr, "[DEBUG] Jump %08X -> %08X\n", orig_pc, RR(001));
        return EIS_OVPC;
    }
    return 0;
}
