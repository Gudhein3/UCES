#include "libasm.h"
#include <assert.h>
#include <errno.h>

const char *regnames[] = {
    "zero",
    "pc",
    "ra",
    "sp",
    "fp",
    "gv",
    "rv",
    "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8",
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10",
    "t0", "t1", "t2", "t3", "t4"
};

#define INST_NOARG (1<<0)
#define INST_NOOUT (1<<1)
#define INST_STND (INST_NOARG|INST_NOOUT)

#define INST_1IMM (1<<2)
#define INST_2IMM (1<<3)

#define DEBUG 0

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
    (AsmInst) { OP_MUL,   0,          "mul"   },
    (AsmInst) { OP_IMUL,  0,          "imul"  },
    (AsmInst) { OP_WR8,   INST_NOOUT, "wr8"   },
    (AsmInst) { OP_WR16,  INST_NOOUT, "wr16"  },
    (AsmInst) { OP_WR32,  INST_NOOUT, "wr32"  },
    (AsmInst) { OP_RD8,   INST_NO2A,  "rd8"   },
    (AsmInst) { OP_RD16,  INST_NO2A,  "rd16"  },
    (AsmInst) { OP_RD32,  INST_NO2A,  "rd32"  },
    (AsmInst) { OP_RDS8,  INST_NO2A,  "rds8"  },
    (AsmInst) { OP_RDS16, INST_NO2A,  "rds16" },
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
    (AsmInst) { OP_LSL,   0,          "lsl"   },
    (AsmInst) { OP_LSR,   0,          "lsr"   },
    (AsmInst) { OP_ANDI,  INST_2IMM,  "andi"  },
    (AsmInst) { OP_ORI,   INST_2IMM,  "ori"   },
    (AsmInst) { OP_DEBUG, INST_STND,  "debug" },
    (AsmInst) { OP_HLT,   INST_STND,  "hlt"   },
    (AsmInst) { OP_UDI,   INST_STND,  "udi"   },
    (AsmInst) { OP_ADDI,  INST_2IMM,  "addi"  },
    (AsmInst) { OP_LDLX,  0,          "ldlx"  },
    (AsmInst) { OP_LDHX,  0,          "ldhx"  },
    (AsmInst) { OP_LDL,   0,          "ldl"   },
    (AsmInst) { OP_LDH,   0,          "ldh"   },

    (AsmInst) { 0,        0,          NULL    } // Terminator
};

#undef panic
#define panic(fmt, ...) do {fprintf(stderr, "%s:%d:\x1b[31mPanic at \""__FILE__":"__STR(__LINE__)"\" \x1b[0m: "fmt"\n", prop.srcfile, prop.lineno, __VA_ARGS__); abort();} while(0)

#define warning(fmt, ...) do {fprintf(stderr, "%s:%d:\x1b[33mWarning at \""__FILE__":"__STR(__LINE__)"\" \x1b[0m: "fmt"\n", prop.srcfile, prop.lineno, __VA_ARGS__); } while(0)

int parse_symbols(SymbolTable *table, const char *fn, u8 *data, size_t size) {
    if (size < 4 || memcmp(data, "UCST", 4) != 0) {
        fprintf(stderr, "Bad symbol table: %s\n", fn);
        return 2;
    }
    size -= 4;
    data += 4;
    while (size != 0) {
        Symbol sym;
        u32 number;
        memcpy(&number, data, 4);
        sym.label.size = number;
        size -= 4;
        data += 4;
        sym.label.data = malloc(sym.label.size);
        memcpy((char *)sym.label.data, data, sym.label.size);
        size -= sym.label.size;
        data += sym.label.size;
        memcpy(&number, data, 4);
        sym.address = number;
        size -= 4;
        data += 4;
        sym.type = ASM_SYMBOL_IMPORTED;
        da_append(table, sym);
    }
    return 0;
}

static int is_token_terminator(int ch) {
    char table[128] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,
    };
    return ch < 128 && table[ch];
}

static String_View asm_chop_token(String_View *line) {
    sv_trim_left(line);
    if (line->size > 0) {
        if (!isspace(line->data[0]) && is_token_terminator(line->data[0])) {
            line->data++;
            line->size--;
            return (String_View) {
                .size = 1,
                .data = line->data - 1,
            };
        }
    }
    String_View token = sv_split_group(line, is_token_terminator);
    // Unchop the terminator.
    line->size++;
    line->data--;
    // TODO: Why do we have to do so?
    // Because I was lazy.
    return token;
}

static AsmInst *_Nullable asm_get_inst(String_View token) {
    for (size_t i = 0; i < sizeof(asm_instructions)/sizeof(*asm_instructions)-1; ++i) {
        if (sv_cmp_cstr(token, asm_instructions[i].name) == 0) return &asm_instructions[i];
    }
    return NULL;
}

static Symbol *_Nullable asm_get_symbol(SymbolTable table, String_View token) {
    for (size_t i = 0; i < table.count; ++i) {
        if (sv_cmp_sv(token, table.items[i].label) == 0) return &table.items[i];
    }
    return NULL;
}

static AsmInst *_Nullable asm_get_inst_by_op(OpCode op) {
    for (size_t i = 0; i < sizeof(asm_instructions)/sizeof(*asm_instructions)-1; ++i) {
        if (asm_instructions[i].op == op) return &asm_instructions[i];
    }
    return NULL;
}

static void asm_expect_token(AsmProp prop, const char *value) {
    String_View token = asm_chop_token(prop.line);
    if (sv_cmp_cstr(token, value))
        panic("Expected token \"%s\", but got \"%.*s\"", value, token.size, token.data);
}

typedef enum {
    BINARY=2,
    OCTAL=8,
    DECIMAL=10,
    HEXADECIMAL=16,
} Base;

static u32 asm_parse_numeric(AsmProp prop, String_View token, Base base) {
    if (token.size < 1) {
        panic("Expected at least one digit in a number", 0);
    }
    u32 num = 0; // TODO: I am pretty sure there is a better solution that doesn't involve copy&pasting the same code 4 times. Unfortunately, I still haven't found it yet.
    switch (base) {
    case BINARY:
        for (size_t i = 0; i < token.size; ++i) {
            char c = token.data[i];
            if ('0' <= c && c <= '1')
                num = num * 2 + c - '0';
            else
                panic("Unexpected digit in a binary literal: '%c'(%02X)\n", c, c);
        }
        break;
    case OCTAL:
        for (size_t i = 0; i < token.size; ++i) {
            char c = token.data[i];
            if ('0' <= c && c <= '7')
                num = num * 8 + c - '0';
            else
                panic("Unexpected digit in an octal literal: '%c'(%02X)\n", c, c);
        }
        break;
    case DECIMAL:
        for (size_t i = 0; i < token.size; ++i) {
            char c = token.data[i];
            if (isdigit(c))
                num = num * 10 + c - '0';
            else
                panic("Unexpected digit in a decimal literal: '%c'(%02X)\n", c, c);
        }
        break;
    case HEXADECIMAL:
        for (size_t i = 0; i < token.size; ++i) {
            char c = token.data[i];
            if (isdigit(c))
                num = num * 16 + c - '0';
            else if (isxdigit(c))
                num = num * 16 + c - 'A' + 10;
            else
                panic("Unexpected digit in a hexadecimal literal: '%c'(%02X)\n", c, c);
        }
        break;
    default:
        assert(0 && "Unreachable: Base");
    }
    return num;
}

static u32 asm_parse_expr(AsmProp prop);

static u32 asm_parse_primary(AsmProp prop) {
    if (prop.line->size < 1)
        panic("Expected at least one symbol in an expression", 0);
    { // Try unary ops first.
        String_View modified_line = *prop.line;
        // Maybe this shouldn't be a case and there should be a peekableish interface.
        // Although, I think the current approach is more flexible in terms of text manipulations.
        String_View token = asm_chop_token(&modified_line);
        if (token.size == 1) {
            switch (token.data[0]) {
            case '+':
                *prop.line = modified_line;
                return asm_parse_primary(prop);
            case '-':
                *prop.line = modified_line;
                return -asm_parse_primary(prop);
            case '!':
                *prop.line = modified_line;
                return !asm_parse_primary(prop);
            case '~':
                *prop.line = modified_line;
                return 0xFFFFFFFF ^ asm_parse_primary(prop);
            }
        }
    }
    String_View token = asm_chop_token(prop.line);
    if (token.size == 0)
        panic("Empty token", 0);
    if (!sv_cmp_cstr(token, "(")) {
        u32 r = asm_parse_expr(prop);
        asm_expect_token(prop, ")");
        return r;
    }
    Symbol *symbol = asm_get_symbol(*prop.symbols, token);
    if (symbol) // Oh heck, it's a symbol
        return symbol->address;

    Base base = DECIMAL;
    if (token.data[0] == '#') { // Oh heck, it's a number with a predefined base.
        token.data++; token.size--;
        if (token.size == 0)
            panic("Empty token", 0);
        switch (token.data[0]) {
        case 'b':
            base = BINARY;
            token.data++; token.size--;
            break;
        case 'o':
            base = OCTAL;
            token.data++; token.size--;
            break;
        case 'd':
            token.data++; token.size--;
            break;
        case 'h':
            base = HEXADECIMAL;
            token.data++; token.size--;
            break;
        default:
            panic("Unknown number prefix: '%c'\n", token.data[0]);
        }
    }
    return asm_parse_numeric(prop, token, base);
}

static u32 asm_parse_e4(AsmProp prop) {
    u32 value = asm_parse_primary(prop);
    for (;;) {
        String_View modified_line = *prop.line;
        String_View token = asm_chop_token(&modified_line);
        if (token.size == 1) {
            switch (token.data[0]) {
            case '*':
                *prop.line = modified_line;
                value *= asm_parse_primary(prop);
                break;
            case '/':
                *prop.line = modified_line;
                value /= asm_parse_primary(prop);
                break;
            case '%':
                *prop.line = modified_line;
                value %= asm_parse_primary(prop);
                break;
            case '&':
                *prop.line = modified_line;
                value &= asm_parse_primary(prop);
                break;
            case '^':
                *prop.line = modified_line;
                value ^= asm_parse_primary(prop);
                break;
            default:
                goto end;
            }
        }
        else {
            break;
        }
    }
    end:
    return value;
}

static u32 asm_parse_e3(AsmProp prop) {
    u32 value = asm_parse_e4(prop);
    for (;;) {
        String_View modified_line = *prop.line;
        String_View token = asm_chop_token(&modified_line);
        if (token.size == 1) {
            switch (token.data[0]) {
            case '|':
                *prop.line = modified_line;
                value |= asm_parse_e4(prop);
                break;
            default:
                goto end;
            }
        }
        else {
            break;
        }
    }
    end:
    return value;
}

static u32 asm_parse_e2(AsmProp prop) {
    u32 value = asm_parse_e3(prop);
    for (;;) {
        String_View modified_line = *prop.line;
        String_View token = asm_chop_token(&modified_line);
        if (token.size == 1) {
            switch (token.data[0]) {
            case '+':
                *prop.line = modified_line;
                value += asm_parse_e3(prop);
                break;
            case '-':
                *prop.line = modified_line;
                value -= asm_parse_e3(prop);
                break;
            default:
                goto end;
            }
        }
        else {
            break;
        }
    }
    end:
    return value;
}

static u32 asm_parse_expr(AsmProp prop) {
    u32 value = asm_parse_e2(prop);
    for (;;) {
        String_View modified_line = *prop.line;
        String_View token = asm_chop_token(&modified_line);
        if (token.size == 1) {
            switch (token.data[0]) {
            case '<':
                *prop.line = modified_line;
                value <<= asm_parse_e2(prop);
                break;
            case '>':
                *prop.line = modified_line;
                value >>= asm_parse_e2(prop);
                break;
            default:
                goto end;
            }
        }
        else {
            break;
        }
    }
    end:
    return value;
}

static int asm_parse_reg(AsmProp prop) {
    String_View tok = asm_chop_token(prop.line);
    if (tok.size > 1 && tok.data[0] == 'x') {
        tok.size--;
        tok.data++;
        int i = asm_parse_numeric(prop, tok, DECIMAL);
        if (i > 0 && i < 32) return i;
        panic("Bad register name: \"%.*s\"", tok.size, tok.data);
    }
    for (size_t i = 0; i < sizeof(regnames)/sizeof(*regnames); ++i) {
        if (sv_cmp_cstr(tok, regnames[i]) == 0) {
            return i;
        }
    }
    panic("Bad register name: \"%.*s\"", tok.size, tok.data);
}

int asm_export_symbols(const char *srcfile, String_View source_code, SymbolTable *table) {
    u32 address = 0;
    int lineno = 0;
    while (source_code.size > 0) {
        lineno++;

        String_View line = sv_split_char(&source_code, '\n');
        line = sv_split_char(&line, ';'); // Remove comments.
        sv_trim_left(&line);
        sv_trim_right(&line);
        if (line.size == 0) continue; // If the first non-space character is ';', the line is one big comment we shall ignore.

        String_View tok = asm_chop_token(&line);
        if (!tok.size) continue;
        AsmProp prop = (AsmProp) {
            .srcfile=srcfile,
            .lineno=lineno,
            .line=&line,
            .symbols=table
        };
        if (tok.data[tok.size-1] == ':') {
            tok.size -= 1;
            SymbolType type = ASM_SYMBOL_LOCAL;
            if (tok.size && tok.data[tok.size-1] == ':') { // "::" at the end of a line means that the symbol is global.
                type = ASM_SYMBOL_GLOBAL;
                tok.size -= 1;
            }
            if (!tok.size) {
                panic("Attempted to create a symbol with no name", NULL);
            }
            Symbol sym = (Symbol) {
                .type = type,
                .label = tok,
                .address = address
            };
            da_append(table, sym);
            continue;
        }
        if (tok.data[0] == '.') {
            if (sv_cmp_cstr(tok, ".db") == 0) {
                address += 1;
            }
            else if (sv_cmp_cstr(tok, ".dp") == 0) {
            }
            else if (sv_cmp_cstr(tok, ".dw") == 0) {
                address += 2;
            }
            else if (sv_cmp_cstr(tok, ".dd") == 0) {
                address += 4;
            }
            else if (sv_cmp_cstr(tok, ".org") == 0) {
                u32 i = asm_parse_expr(prop);
                address = i;
            }
            else if (sv_cmp_cstr(tok, ".ld") == 0) {
                address += 8;
            }
            else if (sv_cmp_cstr(tok, ".if") == 0 ||
                     sv_cmp_cstr(tok, ".ifn") == 0) {
                address += 12;
            }
            else if (sv_cmp_cstr(tok, ".str") == 0) {
                for (size_t i = 0; i < line.size; ++i) {
                    if (line.data[i] == '\\') {
                        ++i;
                        // Assume that the '\' symbol is followed by only one additional symbol.
                        address += 1;
                    }
                    else {
                        address += 1;
                    }
                }
            }
            else if (sv_cmp_cstr(tok, ".incbin") == 0) {
                sv_trim_left(&line);
                char *filename = malloc(line.size + 1);
                if (!filename) {
                    fprintf(stderr, "\x1b[31mFailed to allocate a string: %s\x1b[0m\n", strerror(errno));
                    exit(1);
                }
                memcpy(filename, line.data, line.size);
                filename[line.size] = 0;
                size_t size = get_file_size(filename); // Hopefully nobody will modify the file before the assemble step.
                if (size == -1) {
                    panic("Failed to open the file: %s", strerror(errno));
                }
                free(filename);
                address += size;
            }
            else if (sv_cmp_cstr(tok, ".incsym") == 0) {
                sv_trim_left(&line);
                char *filename = malloc(line.size + 1);
                if (!filename) {
                    fprintf(stderr, "\x1b[31mFailed to allocate a string: %s\x1b[0m\n", strerror(errno));
                    exit(1);
                }
                memcpy(filename, line.data, line.size);
                filename[line.size] = 0;
                size_t size;
                u8 *data = read_file(filename, &size);
                if (!data) {
                    panic("Failed to open file: %s", strerror(errno));
                }
                int status = parse_symbols(table, filename, data, size);
                if (status != 0) {
                    panic("Failed to parse symbol table", 0);
                    exit(status);
                }
                if (size == -1) {
                    panic("Failed to open the file: %s", strerror(errno));
                }
                free(filename);
            }
            else {
                panic("Bad pseudo instruction name: %.*s", tok.size, tok.data);
            }
        }
        else {
            address += 4;
        }
    }
    return 0;
}

int assemble(const char *srcfile, String_View source_code, ByteArray *output, SymbolTable *table) {

    u32 address = 0;
    int lineno = 0;
    while (source_code.size > 0) {
        lineno++;

        String_View line = sv_split_char(&source_code, '\n');
        line = sv_split_char(&line, ';'); // Remove comments.
        sv_trim_left(&line);
        sv_trim_right(&line);
        if (line.size == 0) continue; // If the first non-space character is ';', the line is one big comment we shall ignore.
        if (DEBUG)
            printf("Encountered line: \"%.*s\"\n", line.size, line.data);
        String_View tok = asm_chop_token(&line);
        if (!tok.size) continue;
        if (tok.data[tok.size-1] == ':') {
            continue;
        }
        AsmProp prop = (AsmProp) {
            .srcfile=srcfile,
            .lineno=lineno,
            .line=&line,
            .symbols=table
        };
        if (tok.data[0] == '.') {
            if (sv_cmp_cstr(tok, ".db") == 0) {
                u32 imm = asm_parse_expr(prop);
                if (imm >= 1<<8) {
                    warning("Too large 8 bit constant: h%08X\n", imm);
                }
                da_append(output, imm&0xFF);
            }
            else if (sv_cmp_cstr(tok, ".dp") == 0) {
                assert(0 && "TODO");
            }
            else if (sv_cmp_cstr(tok, ".dw") == 0) {
                u32 imm = asm_parse_expr(prop);
                if (imm >= 1<<16) {
                    warning("Too large 16 bit constant: h%08X\n", imm);
                }
                da_append(output, imm&0xFF);
                da_append(output, (imm>>8)&0xFF);
            }
            else if (sv_cmp_cstr(tok, ".dd") == 0) {
                u32 imm = asm_parse_expr(prop);
                da_append(output, imm&0xFF);
                da_append(output, (imm>>8)&0xFF);
                da_append(output, (imm>>16)&0xFF);
                da_append(output, (imm>>24)&0xFF);
            }
            else if (sv_cmp_cstr(tok, ".org") == 0) {
                u32 i = asm_parse_expr(prop);
                address = i;
            }
            else if (sv_cmp_cstr(tok, ".ld") == 0) {
                u32 imm = asm_parse_expr(prop);
                int a = asm_parse_reg(prop);
                if (a == 001) { // PC
                    warning("Writing to the PC register may lead to PC corruption, due to nature of the '.ld' pseudo instruction. It generates two separated instructions, ldh (i>>16)&0xFFFF reg and ldl i&0xFFFF reg, hopefully, you see why this may cause the corruption", 0);
                }
                // Already 32bit :-)
                da_append(output, OP_LDH);
                da_append(output, (imm>>16)&0xFF);
                da_append(output, (imm>>24)&0xFF);
                da_append(output, a);
                da_append(output, OP_LDL);
                da_append(output, imm&0xFF);
                da_append(output, (imm>>8)&0xFF);
                da_append(output, a);
            }
            else if (sv_cmp_cstr(tok, ".if") == 0 ||
                     sv_cmp_cstr(tok, ".ifn") == 0) {
                int negative = sv_cmp_cstr(tok, ".ifn") == 0;
                int rega = asm_parse_reg(prop);
                String_View cond = asm_chop_token(&line);
                int regb = asm_parse_reg(prop);
                int regx = asm_parse_reg(prop);
                int regy = asm_parse_reg(prop);
                da_append(output, OP_CMP);
                da_append(output, rega);
                da_append(output, regb);
                da_append(output, 27); // t0
                da_append(output, OP_TSBI);
                da_append(output, 27);
                if (sv_cmp_cstr(cond, "o*"))
                    da_append(output, 10);
                else if (sv_cmp_cstr(cond, "c+"))
                    da_append(output, 9);
                else if (sv_cmp_cstr(cond, "c-"))
                    da_append(output, 8);
                else if (sv_cmp_cstr(cond, ">u"))
                    da_append(output, 7);
                else if (sv_cmp_cstr(cond, "<u"))
                    da_append(output, 6);
                else if (sv_cmp_cstr(cond, ">s"))
                    da_append(output, 5);
                else if (sv_cmp_cstr(cond, "<s"))
                    da_append(output, 4);
                else if (sv_cmp_cstr(cond, "!="))
                    da_append(output, 3);
                else if (sv_cmp_cstr(cond, "=="))
                    da_append(output, 2);
                else if (sv_cmp_cstr(cond, "="))
                    da_append(output, 2);
                else
                    panic("Invalid condition: \"%.*s\"", cond.size, cond.data);
                da_append(output, 27); // t0
                if (negative)
                    da_append(output, OP_MVO);
                else
                    da_append(output, OP_MVE);
                da_append(output, 27); // t0
                da_append(output, regx);
                da_append(output, regy); // t0
            }
            else if (sv_cmp_cstr(tok, ".str") == 0) {
                for (size_t i = 0; i < line.size; ++i) {
                    if (line.data[i] == '\\') {
                        ++i;
                        switch (line.data[i]) {
                        case 'n':
                            da_append(output, '\n');
                        break;
                        case 'r':
                            da_append(output, '\r');
                        break;
                        case 't':
                            da_append(output, '\t');
                        break;
                        case '0':
                            da_append(output, '\0');
                        break;
                        }
                    }
                    else {
                        da_append(output, line.data[i]);
                    }
                }
            }
            else if (sv_cmp_cstr(tok, ".incbin") == 0) {
                sv_trim_left(&line);
                char *filename = malloc(line.size + 1);
                if (!filename) {
                    fprintf(stderr, "\x1b[31mFailed to allocate %zu bytes: %s\x1b[0m\n", line.size + 1, strerror(errno));
                    exit(1);
                }
                memcpy(filename, line.data, line.size);
                filename[line.size] = 0;
                size_t size;
                u8 *data = read_file(filename, &size);
                if (!data) {
                    panic("Failed to open file: %s", strerror(errno));
                }
                free(filename);
                da_extend(output, size, data);
                free(data);
            }
            else if (sv_cmp_cstr(tok, ".incsym") == 0) {
            }
            else {
                panic("Bad pseudo instruction name: %.*s", tok.size, tok.data);
            }
        }
        else {
            AsmInst *inst = asm_get_inst(tok);
            if (!inst) {
                panic("Bad instruction name: %.*s", tok.size, tok.data);
            }

            // [Opcode:8] [R1:8] [R2:8] [R3:8]
            // [Opcode:8] [IMM:16]      [R3:8]
            OpCode op = inst->op;
            da_append(output, op);

            if (DEBUG)
                printf("Produced opcode %02X\n", op);

            if ((op & 0xF0) == 0xF0) {
                u32 imm = asm_parse_expr(prop);
                if (imm >= 1<<16) {
                    warning("Too large 16 bit constant: h%08X\n", imm);
                }
                da_append(output, imm&0xFF);
                da_append(output, (imm >> 8)&0xFF);
            }
            else if (inst->flags & INST_NOARG) { // Indeed no arguments.
                da_append(output, 0);
                da_append(output, 0);
            }
            else {
                if (inst->flags & INST_1IMM) {
                    u32 imm = asm_parse_expr(prop);
                    if (imm >= 1<<8) {
                        warning("Too large 8 bit constant: h%08X\n", imm);
                    }
                    da_append(output, imm&0xFF);
                }
                else {
                    int reg = asm_parse_reg(prop);
                    da_append(output, reg);
                    if (DEBUG)
                        printf("Produced REG1 %02X\n", reg);
                }

                if (inst->flags & INST_2IMM) {
                    u32 imm = asm_parse_expr(prop);
                    if (imm >= 1<<8) {
                        warning("Too large 8 bit constant: h%08X\n", imm);
                    }
                    da_append(output, imm&0xFF);
                }
                else if (inst->flags & INST_NO2A) {
                    da_append(output, 0);
                }
                else {
                    int reg = asm_parse_reg(prop);
                    da_append(output, reg);
                    if (DEBUG)
                        printf("Produced REG2 %02X\n", reg);
                }
            }
            if (inst->flags & INST_NOOUT) {
                // No output indeed.
                da_append(output, 0);
            }
            else {
                int reg = asm_parse_reg(prop);
                da_append(output, reg);
                if (DEBUG)
                    printf("Produced REG3 %02X\n", reg);
            }
        }
    }
    return 0;
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
        // If the instruction seems to be invalid, treat it as dd.
        if (inst == NULL ||
            cr >= 32 ||
            ((op & 0xF0) != 0xF0 && !(inst->flags & INST_1IMM) && ar >= 32) ||
            ((op & 0xF0) != 0xF0 && !(inst->flags & INST_2IMM) && br >= 32)) {
            sb_printf(sb, ".dd 0x%02X%02X%02X%02X\n", bin.data[pc+3], bin.data[pc+2], bin.data[pc+1], bin.data[pc]);
            pc += 4;
            continue;
        }
        size_t curr_sb_size = sb->count;
        sb_printf(sb, "%s", inst->name);
        if ((op & 0xF0) == 0xF0) {
            sb_printf(sb, " %u", imm);
        }
        else if (inst->flags & INST_NOARG) {
            // No args so nothing interesting here.
        }
        else {
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
        sb_printf(sb, "%*s; h%02X h%02X h%02X h%02X at h%02X\n", 40-(sb->count-curr_sb_size), "", bin.data[pc], bin.data[pc+1], bin.data[pc+2], bin.data[pc+3], pc);
        pc += 4;
    }

    return 0;
}
