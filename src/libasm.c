#include "libasm.h"
#include <assert.h>

static const char *regnames[] = {
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

#define INST_NOARG (1<<0)
#define INST_NOOUT (1<<1)
#define INST_STND (INST_NOARG|INST_NOOUT)

#define INST_1IMM (1<<2)
#define INST_2IMM (1<<3)

AsmInst asm_instructions[] = { // Terminated with (AsmInst) {0, 0, NULL}
    (AsmInst) { OP_ADD,   0,          "add"   },
    (AsmInst) { OP_SUB,   0,          "sub"   },
    (AsmInst) { OP_AND,   0,          "and"   },
    (AsmInst) { OP_OR,    0,          "or"    },
    (AsmInst) { OP_XOR,   0,          "xor"   },
    (AsmInst) { OP_NAND,  0,          "nand"  },
    (AsmInst) { OP_NOR,   0,          "nor"   },
    (AsmInst) { OP_XNOR,  0,          "xnor"  },
    (AsmInst) { OP_NEG,   0,          "neg"   },
    (AsmInst) { OP_NOT,   0,          "not"   },
    (AsmInst) { OP_DIV,   0,          "div"   },
    (AsmInst) { OP_IDIV,  0,          "idiv"  },
    (AsmInst) { OP_REM,   0,          "rem"   },
    (AsmInst) { OP_IREM,  0,          "irem"  },
    (AsmInst) { OP_WR8,   INST_NOOUT, "wr8"   },
    (AsmInst) { OP_WR16,  INST_NOOUT, "wr16"  },
    (AsmInst) { OP_WR32,  INST_NOOUT, "wr32"  },
    (AsmInst) { OP_RD8,   0,          "rd8"   },
    (AsmInst) { OP_RD16,  0,          "rd16"  },
    (AsmInst) { OP_RD32,  0,          "rd32"  },
    (AsmInst) { OP_RDS8,  0,          "rds8"  },
    (AsmInst) { OP_RDS16, 0,          "rds16" },
    (AsmInst) { OP_CMP,   0,          "cmp"   },
    (AsmInst) { OP_MV,    INST_NO2A,  "mv"    },
    (AsmInst) { OP_MVE,   0,          "mve"   },
    (AsmInst) { OP_MVO,   0,          "mvo"   },
    (AsmInst) { OP_TSBI,  INST_2IMM,  "tsbi"  },
    (AsmInst) { OP_TSI,   INST_2IMM,  "tsi"   },
    (AsmInst) { OP_TSB,   0,          "tsb"   },
    (AsmInst) { OP_TS,    0,          "ts"    },
    (AsmInst) { OP_CALL,  INST_SING,  "call"  },
    (AsmInst) { OP_RET,   INST_STND,  "ret"   },
    (AsmInst) { OP_PUSH,  INST_SING,  "push"  },
    (AsmInst) { OP_POP,   INST_NOARG, "pop"   },
    (AsmInst) { OP_ANDI,  INST_2IMM,  "andi"  },
    (AsmInst) { OP_ORI,   INST_2IMM,  "ori"   },
    (AsmInst) { OP_WAIT,  INST_STND,  "wait"  },
    (AsmInst) { OP_HLT,   INST_STND,  "hlt"   },
    (AsmInst) { OP_UDI,   INST_STND,  "udi"   },
    (AsmInst) { OP_ADDI,  INST_2IMM,  "addi"  },
    (AsmInst) { OP_LDLX,  0,          "ldlx"  },
    (AsmInst) { OP_LDHX,  0,          "ldhx"  },
    (AsmInst) { OP_LDL,   0,          "ldl"   },
    (AsmInst) { OP_LDH,   0,          "ldh"   },

    (AsmInst) { 0,        0,          NULL    } // Terminator
};

// TODO: Add support for linking with external symbols

static _Thread_local const char *srcfile;
static _Thread_local int lineno;
static _Thread_local ByteArray instcode;
static _Thread_local SymbolTable *symbols;
static _Thread_local size_t onaddress; // Address on which pc currently should be if the program is loaded correctly to memory.

#define panic(fmt, ...) do {fprintf(stderr, "\x1b[31mPanic at \""__FILE__":"__STR(__LINE__)"\" for \"%s:%d\"\x1b[0m: "fmt"\n", srcfile, lineno+1, __VA_ARGS__); abort();} while(0)

#define warning(fmt, ...) do {fprintf(stderr, "\x1b[33mWarning at \""__FILE__":"__STR(__LINE__)"\" for \"%s:%d\"\x1b[0m: "fmt"\n", srcfile, lineno+1, __VA_ARGS__); } while(0)

u32 asm_parse_numeric(String_View token) {
    if (token.size == 0) {
        panic("Expected numeric but got empty token\n", NULL);
    }
    u32 i = 0;
    size_t idx = 0;
    int neg = 0;
    if (token.data[0] == '-') {
        idx += 1;
        neg = 1;
    }
    int system = 0;
    switch (tolower(token.data[0])) {
    case 'h':
        system = 1;
        idx += 1;
        break;
    case 'b':
        system = 2;
        idx += 1;
        break;
    case 'o':
        system = 3;
        idx += 1;
        break;
    case 'd': // Explicitly use decimal system
    default: // Implicitly use decimal system
    }
    while (idx < token.size) {
        if (system == 0) {
            if (isdigit(token.data[idx])) {
                i = i*10+token.data[idx]-'0';
            }
        }
        else if (system == 1) {
            if ('0' <= token.data[idx] && token.data[idx] <= '9') {
                i = i*16+token.data[idx]-'0';
            }
            else if ('a' <= token.data[idx] && token.data[idx] <= 'f') {
                i = i*16+token.data[idx]-'a'+10;
            }
            else if ('A' <= token.data[idx] && token.data[idx] <= 'F') {
                i = i*16+token.data[idx]-'A'+10;
            }
        }
        else if (system == 2) {
            if ('0' <= token.data[idx] && token.data[idx] <= '1') {
                i = i*2+token.data[idx]-'0';
            }
        }
        else if (system == 3) {
            if ('0' <= token.data[idx] && token.data[idx] <= '7') {
                i = i*8+token.data[idx]-'0';
            }
        }
        idx += 1;
    }
    return neg ? -i : i;
}

static int asm_parse_primary_terminator(int x) {
    return isspace(x) || x == '+' || x == '-' || x == '*' || x == '/' || x == '|' || x == '&' || x == '^';
}

u32 asm_parse_primary(String_View *line) {
    String_View token = sv_split_group(line, asm_parse_primary_terminator);
    if (token.size == 0) {
        panic("Expected numeric or symbol name but got empty token\n", NULL);
    }
    if (symbols) {
        for (size_t i = 0; i < symbols->count; ++i) {
            if (sv_cmp_sv(token, symbols->items[i].label) == 0) {
                return symbols->items[i].address;
            }
        }
    }
    u32 i = 0;
    size_t idx = 0;
    int neg = 0;
    if (token.data[0] == '-') {
        idx += 1;
        neg = 1;
    }
    int system = 0;
    switch (tolower(token.data[0])) {
    case 'h':
        system = 1;
        idx += 1;
        break;
    case 'b':
        system = 2;
        idx += 1;
        break;
    case 'o':
        system = 3;
        idx += 1;
        break;
    case 'd': // Explicitly use decimal system
    default: // Implicitly use decimal system
    }
    while (idx < token.size) {
        if (system == 0) {
            if (isdigit(token.data[idx])) {
                i = i*10+token.data[idx]-'0';
            }
        }
        else if (system == 1) {
            if ('0' <= token.data[idx] && token.data[idx] <= '9') {
                i = i*16+token.data[idx]-'0';
            }
            else if ('a' <= token.data[idx] && token.data[idx] <= 'f') {
                i = i*16+token.data[idx]-'a'+10;
            }
            else if ('A' <= token.data[idx] && token.data[idx] <= 'F') {
                i = i*16+token.data[idx]-'A'+10;
            }
        }
        else if (system == 2) {
            if ('0' <= token.data[idx] && token.data[idx] <= '1') {
                i = i*2+token.data[idx]-'0';
            }
        }
        else if (system == 3) {
            if ('0' <= token.data[idx] && token.data[idx] <= '7') {
                i = i*8+token.data[idx]-'0';
            }
        }
        idx += 1;
    }
    return neg ? -i : i;
}

u32 asm_parse_expr(String_View *line) {
    return asm_parse_primary(line);
}

u32 asm_get_imm(String_View *line) {
    u32 i = asm_parse_expr(line);
    if (i >= (1<<16)) {
        warning("Too big numeric literal", NULL);
    }
    return i;
}

int asm_get_reg(String_View token) {
    if (token.size > 1 && token.data[0] == 'x') {
        token.size--;
        token.data++;
        int i = asm_get_imm(&token);
        if (i > 0 && i < 32) return i;
        panic("Bad register name: \"%.*s\"", token.size, token.data);
    }
    for (size_t i = 0; i < sizeof(regnames)/sizeof(*regnames); ++i) {
        if (sv_cmp_cstr(token, regnames[i]) == 0) {
            return i;
        }
    }
    panic("Bad register name: \"%.*s\"", token.size, token.data);
}

AsmInst *_Nullable asm_get_inst(String_View token) {
    for (size_t i = 0; i < sizeof(asm_instructions)/sizeof(*asm_instructions)-1; ++i) {
        if (sv_cmp_cstr(token, asm_instructions[i].name) == 0) return &asm_instructions[i];
    }
    return NULL;
}

AsmInst *_Nullable asm_get_inst_by_op(OpCode op) {
    for (size_t i = 0; i < sizeof(asm_instructions)/sizeof(*asm_instructions)-1; ++i) {
        if (asm_instructions[i].op == op) return &asm_instructions[i];
    }
    return NULL;
}

static u8 *_Nullable asm_read_file(const char *filename, size_t *_Nullable size) {
    FILE *fptr = fopen(filename, "rb");
    if (!fptr) return NULL;
    size_t _size;
    fseek(fptr, 0, SEEK_END);
    _size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    if (size) *size = _size;
    u8 *buf = malloc(_size);
    if (!buf) {
        fclose(fptr);
        return NULL;
    }
    fread(buf, _size, 1, fptr);
    fclose(fptr);
    return buf;
}

// #define DEBUG

int asm_export_symbols(String_View source_code, SymbolTable *table) {
    lineno = 0;
    onaddress = 0;

    while (source_code.size > 0) {
        String_View line = sv_split_char(&source_code, '\n');
        line = sv_split_char(&line, ';');
        sv_trim_left(&line);
        sv_trim_right(&line);
        if (line.size == 0) continue;

        String_View tok = sv_split_group(&line, isspace); // Chopping a token.

        if (tok.size && tok.data[tok.size-1] == ':') {
            tok.size -= 1;
            SymbolType type = ASM_SYMBOL_LOCAL;
            if (tok.size && tok.data[tok.size-1] == ':') { // Ending with "::" makes the symbol global.
                type = ASM_SYMBOL_GLOBAL;
                tok.size -= 1;
            }
            if (!tok.size) {
                panic("Attempted to create a symbol with no name", NULL);
            }
            Symbol sym = (Symbol) {
                .type = type,
                .label = tok,
                .address = onaddress
            };
            da_append(table, sym);
            continue;
        }

        sv_trim_left(&line);
        size_t instsize = 0;
        if (tok.size && tok.data[0] == '.') { // Pseudo instructions
            if (sv_cmp_cstr(tok, ".db")) {
                instsize += 1;
            }
            else if (sv_cmp_cstr(tok, ".dw")) {
                instsize += 2;
            }
            else if (sv_cmp_cstr(tok, ".dd")) {
                instsize += 4;
            }
            else if (sv_cmp_cstr(tok, ".org")) {
                tok = sv_split_group(&line, isspace);
                u32 i = asm_parse_numeric(tok);
                onaddress = i;
            }
            else {
                panic("Bad pseudo instruction name: %.*s", tok.size, tok.data);
            }
        }
        else { // Normal instructions
            AsmInst *inst = asm_get_inst(tok); // Just checking the code is valid.
            if (inst == NULL) {
                panic("Bad instruction name: %.*s", tok.size, tok.data);
            }

            instsize += 4;
        }

        onaddress += instsize;
        lineno += 1;
    }

    return 0;
}

int assemble(String_View source_code, ByteArray *output, SymbolTable _Nullable *symbols_table) {
    lineno = 0;
    onaddress = 0;
    memset(&instcode, 0, sizeof(instcode));
    symbols = symbols_table;

    while (source_code.size > 0) {
        String_View line = sv_split_char(&source_code, '\n');
        #ifdef DEBUG
        printf("f %.*s\n", line.size, line.data);
        #endif
        line = sv_split_char(&line, ';');
        sv_trim_left(&line);
        sv_trim_right(&line);
        if (line.size == 0) continue;

        String_View tok = sv_split_group(&line, isspace); // Chopping a token.

        #ifdef DEBUG
        printf("i %.*s\n", line.size, line.data);
        #endif

        if (tok.size && tok.data[tok.size-1] == ':') {
            continue;
        }

        sv_trim_left(&line);
        instcode.count = 0;
        if (tok.size && tok.data[0] == '.') { // Pseudo instructions
            if (sv_cmp_cstr(tok, ".db")) {
                u32 i = asm_parse_expr(&line);
                if (i >= (1<<8)) {
                    warning("Too big 8bit numeric literal", NULL);
                }
                da_append(&instcode, i);
            }
            else if (sv_cmp_cstr(tok, ".dw")) {
                u32 i = asm_parse_expr(&line);
                if (i >= (1<<16)) {
                    warning("Too big 16bit numeric literal", NULL);
                }
                da_append(&instcode, i&0xFF);
                da_append(&instcode, (i>>8));
            }
            else if (sv_cmp_cstr(tok, ".dd")) {
                u32 i = asm_parse_expr(&line);
                da_append(&instcode, i&0xFF);
                da_append(&instcode, (i>>8)&0xFF);
                da_append(&instcode, (i>>16)&0xFF);
                da_append(&instcode, (i>>24));
            }
            else if (sv_cmp_cstr(tok, ".org")) {
                u32 i = asm_parse_expr(&line);
                onaddress = i;
            }
            else {
                panic("Bad pseudo instruction name: %.*s", tok.size, tok.data);
            }
        }
        else { // Normal instructions
            AsmInst *inst = asm_get_inst(tok);
            if (inst == NULL) {
                panic("Bad instruction name: %.*s", tok.size, tok.data);
            }

            OpCode op = inst->op; // First byte is an opcode.
            da_append(&instcode, op);

            #ifdef DEBUG
            printf("0 %.*s\n", line.size, line.data);
            #endif
            if ((op & 0xF0) == 0xF0) {
                u16 imm = (u16)(u64)asm_get_imm(&line);
                #ifdef DEBUG
                printf("a %.*s\n", line.size, line.data);
                #endif
                da_append(&instcode, (imm>> 0)&0xFF);
                da_append(&instcode, (imm>> 8)&0xFF);
            }
            else if (inst->flags & INST_NOARG) {
                // No args so nothing interesting here.
                da_append(&instcode, 0);
                da_append(&instcode, 0);
            }
            else {
                // TODO: Make it cleaner.

                if (inst->flags & INST_1IMM) {
                    u16 imm = (u16)(u64)asm_get_imm(&line);
                    da_append(&instcode, imm); // We need only lower 8 bits.
                }
                else {
                    tok = sv_split_group(&line, isspace); // 1st argument
                    int reg = asm_get_reg(tok);
                    da_append(&instcode, reg);
                }
                #ifdef DEBUG
                printf("a %.*s\n", line.size, line.data);
                #endif

                if (inst->flags & INST_2IMM) {
                    u16 imm = (u16)(u64)asm_get_imm(&line);
                    da_append(&instcode, imm); // We need only lower 8 bits.
                }
                else if (inst->flags & INST_NO2A) {
                    da_append(&instcode, 0);
                }
                else {
                    tok = sv_split_group(&line, isspace); // 2st argument
                    int reg = asm_get_reg(tok);
                    da_append(&instcode, reg);
                }
                #ifdef DEBUG
                printf("b %.*s\n", line.size, line.data);
                #endif
            }

            if (inst->flags & INST_NOOUT) {
                // No output so nothing interesting here.
                da_append(&instcode, 0);
            }
            else {
                tok = sv_split_group(&line, isspace); // Output
                #ifdef DEBUG
                printf("1 %.*s\n", line.size, line.data);
                #endif
                int reg = asm_get_reg(tok);
                da_append(&instcode, reg);
            }
        }
        #ifdef DEBUG
        printf("%02X", instcode.items[0]);
        for (int i = 1; i < instcode.count; ++i) {
            printf("-%02X", instcode.items[i]);
        }
        printf("\n");
        printf("%.*s\n", line.size, line.data);
        #endif

        da_extend(output, instcode.count, instcode.items);
        onaddress += instcode.count;
        // fwrite(instcode.items, 1, instcode.count, output_file);
        lineno += 1;
    }

    if (instcode.items) free(instcode.items);
    return 0;
}

#include <stdarg.h>

// TODO: Move to its own module.
static void sb_printf(String_Builder *sb, const char *fmt, ...) {
    va_list list0, list1;
    va_start(list0, fmt);
    va_copy(list1, list0);
    size_t cnt = vsnprintf(NULL, 0, fmt, list0);
    char *buf = malloc(cnt+1);
    if (buf == NULL) {
        fprintf(stderr, "Failed to allocate %zu bytes: BUY MORE RAM FOR $499.89!!!\n", cnt+1);
        exit(1);
    }
    vsnprintf(buf, cnt+1, fmt, list1);
    da_extend(sb, cnt, buf);
    free(buf);
    va_end(list0);
    va_end(list1);
}

int unassemble(Byte_View bin, String_Builder *sb) {
    size_t pc = 0;
    while (pc < bin.size) {
        OpCode op = bin.data[pc];
        AsmInst *inst = asm_get_inst_by_op(op);
        unsigned ar = bin.data[pc+1];
        unsigned br = bin.data[pc+2];
        unsigned imm = (br<<8)|ar;
        unsigned cr = bin.data[pc+3];
        if (inst == NULL ||
            cr >= 32 ||
            ((op & 0xF0) != 0xF0 && !(inst->flags & INST_1IMM) && ar >= 32) ||
            ((op & 0xF0) != 0xF0 && !(inst->flags & INST_2IMM) && br >= 32)) {
            sb_printf(sb, ".dd 0x%02X%02X%02X%02X\n", bin.data[pc+3], bin.data[pc+2], bin.data[pc+1], bin.data[pc]);
            pc += 4;
            continue;
        }
        sb_printf(sb, "%s", inst->name);
        if ((op & 0xF0) == 0xF0) {
            sb_printf(sb, " %u", imm);
        }
        else if (inst->flags & INST_NOARG) {
            // No args so nothing interesting here.
        }
        else {
            // TODO: Make it cleaner.

            // 1st argument
            if (inst->flags & INST_1IMM) {
                sb_printf(sb, " %d", ar);
            }
            else {
                sb_printf(sb, " %s", regnames[ar]);
            }

            // 2st argument
            if (inst->flags & INST_2IMM) {
                sb_printf(sb, " %d", br);
            }
            else if (inst->flags & INST_NO2A) {
                // No second argument so nothing interesting here.
            }
            else {
                sb_printf(sb, " %s", regnames[br]);
            }
        }

        if (inst->flags & INST_NOOUT) {
            // No output so nothing interesting here.
        }
        else {
            sb_printf(sb, " %s", regnames[cr]);
        }
        // TODO: make column adjustable.
        sb_printf(sb, "\x1b[40G; h%02X h%02X h%02X h%02X\n", bin.data[pc+3], bin.data[pc+2], bin.data[pc+1], bin.data[pc]);
        pc += 4;
    }

    return 0;
}
