#include "uces.h"
#include "libasm.h"

static int dmem_poke(u32 addr, u8 value) {
    if (addr < vm.memory_size) {
        vm.memory[addr] = value;
        return 1;
    }
    return 0;
}

static int dmem_peek(u32 addr, u8 *result) {
    if (addr < vm.memory_size) {
        *result = vm.memory[addr];
        return 1;
    }
    return 0;
}

static Dev dmem_device = {dmem_poke, dmem_peek};

extern Dev uart_device;

Dev *devices[] = {
    &dmem_device,
    &uart_device
};

VM vm;

u32 RR(u8 r) {
    if (r == 0) return 0;
    if (r == 1) return vm.pc;
    return vm.registers[r-2];
}

static int is_pc_rewritten = 0;

void WR(u8 r, u32 d) {
    if (r == 0) return;
    if (r == 1) {
        vm.pc = d;
        is_pc_rewritten = 1;
    }
    else vm.registers[r-2] = d;
}

void WRl(u8 r, u16 d) {
    if (r == 0) return;
    if (r == 1) {
        vm.pc = vm.pc&0xFFFF0000|d;
        is_pc_rewritten = 1;
    }
    else vm.registers[r-2] = vm.registers[r-2]&0xFFFF0000|d;
}

void WRh(u8 r, u16 d) {
    if (r == 0) return;
    if (r == 1) {
        vm.pc = vm.pc&0x0000FFFF|(d<<16);
        is_pc_rewritten = 1;
    }
    else vm.registers[r-2] = vm.registers[r-2]&0x0000FFFF|(d<<16);
}

const char *regnames[] = {
    "zero",
    "pc",
    "ra",
    "sp",
    "fp",
    "gv",
    "rv",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8",
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

void dump() {
    printf("---- DUMP ----\n");
    printf("Registers:\n");
    for (int i = 0; i < 32; ++i) {
        printf("x%d,%s: 0x%04X\n", i, regnames[i], RR(i));
    }
}

#define _STR0(x) #x
#define _STR(x) _STR0(x)
#define fault(fmt, ...) do {dump(); fprintf(stderr, "fault " __FILE__ "," _STR(__LINE__) ": " fmt"\n", __VA_ARGS__); abort();} while(0)
#define panic(fmt, ...) do {fprintf(stderr, "panic " __FILE__ "," _STR(__LINE__) ": " fmt"\n", __VA_ARGS__); abort();} while(0)

#define LITTLE_ENDIAN 0
#define ENDIAN_BIG 1

u32 PEEK8(u32 addr) {
    for (int i = 0; i < sizeof(devices)/sizeof(*devices); ++i) {
        u8 a = 0;
        if (devices[i]->peek(addr, &a)) return a;
    }
    fault("Memory address %p to a byte doesn't belong to any device", addr);
}

u32 PEEK16(u32 addr) {
    if (addr+1 < addr) // Avoid overflows.
        fault("Invalid memory address to a word: %p: Overflow", addr);
    for (int i = 0; i < sizeof(devices)/sizeof(*devices); ++i) {
        u8 a = 0, b = 0;
        if (devices[i]->peek(addr, &a) && devices[i]->peek(addr+1, &b)) return (b<<8) | a;
    }
    fault("Memory address %p to a word doesn't belong to any device", addr);
}

u32 PEEK32(u32 addr) {
    if (addr+3 < addr) // Avoid overflows.
        fault("Invalid memory address to a word: %p: Overflow", addr);
    for (int i = 0; i < sizeof(devices)/sizeof(*devices); ++i) {
        u8 a = 0, b = 0, c = 0, d = 0;
        if (devices[i]->peek(addr, &a) &&
            devices[i]->peek(addr+1, &b) &&
            devices[i]->peek(addr+2, &c) &&
            devices[i]->peek(addr+3, &d)) {
                u32 r = (d<<24) | (c<<16) | (b<<8) | a;
                // printf("*%08X -> %08X\n", addr, r);
                return r;
        }
    }
    fault("Memory address %p to a dword doesn't belong to any device", addr);
}

u32 PEEKS8(u32 addr) {
    return (u32)(s32)(s8)PEEK8(addr);
}

u32 PEEKS16(u32 addr) {
    return (u32)(s32)(s16)PEEK16(addr);
}

void POKE8(u32 addr, u32 b) {
    for (int i = 0; i < sizeof(devices)/sizeof(*devices); ++i) {
        if (devices[i]->poke(addr, b)) return;
    }
    fault("Memory address %p to a byte doesn't belong to any device", addr);
}

void POKE16(u32 addr, u32 b) {
    if (addr+1 < addr) // Avoid overflows.
        fault("Invalid memory address to a word: %p: Overflow", addr);
    for (int i = 0; i < sizeof(devices)/sizeof(*devices); ++i) {
        if (devices[i]->poke(addr, b) && devices[i]->poke(addr+1, b>>8)) return;
    }
    fault("Memory address %p to a word doesn't belong to any device", addr);
}

void POKE32(u32 addr, u32 b) {
    // printf("%08X -> *%08X\n", b, addr);
    if (addr+3 < addr) // Avoid overflows.
        fault("Invalid memory address to a word: %p: Overflow", addr);
    for (int i = 0; i < sizeof(devices)/sizeof(*devices); ++i) {
        if (devices[i]->poke(addr, b) &&
            devices[i]->poke(addr+1, b>>8) &&
            devices[i]->poke(addr+2, b>>16) &&
            devices[i]->poke(addr+3, b>>24)) return;
    }
    fault("Memory address %p to a word doesn't belong to any device", addr);
}

#define MW8 POKE8
#define MW16 POKE16
#define MW32 POKE32

#define MR8 PEEK8
#define MR16 PEEK16
#define MR32 PEEK32

#define MRS8 PEEKS8
#define MRS16 PEEKS16

typedef enum {
    EIS_HALT = 1<<0,
    EIS_OVPC = 1<<1,
} EIS;

int evalute_instruction() {
    is_pc_rewritten = 0;
    OpCode op = PEEK8(vm.pc);
    u8 ar, br, cr;
    u32 a, b;
    ar = PEEK8(vm.pc+1);
    br = PEEK8(vm.pc+2);
    cr = PEEK8(vm.pc+3);
    a = RR(ar);
    b = RR(br);
    u16 imm = ((u16)br<<8)|(u16)ar;

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
    case OP_WAIT:
        dump();
        printf("Press enter to continue: ");
        fflush(stdout);
        getchar();
        break;
    case OP_HLT:
        return EIS_HALT;
    case OP_ADDI: WR(cr, a+br);
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
        fault("Illegal instruction: 0x%02X at 0x%04X", op, vm.pc);
    }
    if (is_pc_rewritten == 1) {
        return EIS_OVPC;
    }
    return 0;
}

#define KIB(k, b) (k)*1024+(b)
#define MIB(m, b) KIB((m)*1024,b)
#define GIB(g, b) GIB((g)*1024,b)
#define MEMSIZE MIB(12, 0)
#define STACK_START (MEMSIZE-KIB(2,0))

#define CSize_Fmt "%uGiB %uMiB %uKiB %uB"

#define CSIZE_FORMAT(c) ((c)/1024/1024/1024), (((c)/1024/1024)%1024), (((c)/1024)%1024), ((c)%1024)

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

void usage(char *prog) {
    fprintf(stderr, "usage: %s <binary UCES executable>\n", prog);
}

int main(int argc, char **argv) {
    s32 botest;
    memcpy(&botest, "ABCD", 4);
    if (botest != 0x44434241) {
        fprintf(stderr, "Sorry, but this software doesn't support big-endian systems.\n");
        return 1;
    }
    if (argc != 2) {
        usage(argv[0]);
        fprintf(stderr, "Expected at 1 argument but got %d\n", argc-1);
        return 2;
    }
    size_t program_size;
    u8 *program = read_file(argv[1], &program_size);
    if (!program) panic("Failed to read program.\n", NULL);
    if (program_size > MEMSIZE) panic("Program too big:\n\tProgram size is {"CSize_Fmt"} while Memory size is {"CSize_Fmt"}", CSIZE_FORMAT(program_size), CSIZE_FORMAT(MEMSIZE));
    vm.memory = calloc(1, MEMSIZE);
    vm.memory_size = MEMSIZE;
    vm.registers[1] = STACK_START;
    memcpy(vm.memory, program, program_size);
    free(program);
    int status;
    String_Builder inst_deasm = {0};
    for (;;) {
        // inst_deasm.count = 0;
        // unassemble((Byte_View) {vm.memory+vm.pc, 4}, &inst_deasm);
        // printf("\n0x%08X: %.*s", vm.pc, inst_deasm.count, inst_deasm.items);
        status = evalute_instruction();
        if (status & EIS_HALT) break;
        if ((status & EIS_OVPC) == 0) vm.pc += 4;
    }
}
