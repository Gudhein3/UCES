#include "libasm.h"

void usage(char *prog) {
    fprintf(stderr, "usage: %s:\n"
                    "\tasm [options] <input code> <output binary>\n"
                    "\tsym <input code> <output symbol table>\n"
                    "\tunasm <input binary>\n"
                    "Options:\n"
                    "\t-l <symbol table>   Import symbols from the table\n", prog);
}

int main(int argc, char **argv) {
    s32 botest;
    memcpy(&botest, "ABCD", 4);
    if (botest != 0x44434241) {
        fprintf(stderr, "Sorry, but this software doesn't support big-endian systems.\n");
        return 1;
    }

    if (argc < 2) {
        fprintf(stderr, "Expected at least 1 argument but got %d\n", argc-1);
        usage(argv[0]);
        return 2;
    }

    if (strcmp(argv[1], "asm") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Expected 3 arguments but got %d\n", argc-1);
            usage(argv[0]);
            return 2;
        }
        FILE *output_file = fopen(argv[3], "wb");
        if (output_file == NULL) {
            fprintf(stderr, "Failed to open output file \"%s\" for writing\n", argv[3]);
            return 2;
        }
        // TODO(20260925-122535)
        size_t src_size; // Source code -> String View
        char *src = (char *)read_file(argv[2], &src_size);
        if (src == NULL) {
            if (src_size == 0) { // Command line argument related issue
                fprintf(stderr, "Failed to open source file: \"%s\"\n", argv[2]);
                return 2;
            }
            // System related issue
            fprintf(stderr, "Failed to read source file: \"%s\"\n", argv[2]);
            return 1;
        }
        String_View sv = (String_View) {
            .data = src,
            .size = src_size
        };
        ByteArray instcode = {0};
        SymbolTable symtable = {0};
        int retcode = asm_export_symbols(sv, &symtable);
        if (retcode != 0) return retcode;
        retcode = assemble(sv, &instcode, &symtable);
        if (retcode != 0) return retcode;
        fwrite(instcode.items, 1, instcode.count, output_file);
        return 0;
    }
    if (strcmp(argv[1], "sym") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Expected 3 arguments but got %d\n", argc-1);
            usage(argv[0]);
            return 2;
        }
        FILE *output_file = fopen(argv[3], "wb");
        if (output_file == NULL) {
            fprintf(stderr, "Failed to open output file \"%s\" for writing\n", argv[3]);
            return 2;
        }

        size_t src_size; // Source code -> String View
        char *src = (char *)read_file(argv[2], &src_size);
        if (src == NULL) {
            if (src_size == 0) { // Command line argument related issue
                fprintf(stderr, "Failed to open source file: \"%s\"\n", argv[2]);
                return 2;
            }
            // System related issue
            fprintf(stderr, "Failed to read source file: \"%s\"\n", argv[2]);
            return 1;
        }
        String_View sv = (String_View) {
            .data = src,
            .size = src_size
        };
        SymbolTable symtable = {0};
        int retcode = asm_export_symbols(sv, &symtable);
        if (retcode != 0) return retcode;
        fprintf(output_file, "UCST");
        int any = 0;
        for (size_t i = 0; i < symtable.count; ++i) {
            Symbol *sym = &symtable.items[i];
            if (sym->type == ASM_SYMBOL_GLOBAL) {
                fwrite(&sym->label.size, 1, 4, output_file);
                fwrite(sym->label.data,  1, sym->label.size, output_file);
                fwrite(&sym->address,    1, 4, output_file);
                any = 1;
            }
        }
        if (!any)
            fprintf(stderr, "\x1b[33mWarning at \""__FILE__":"__STR(__LINE__)"\"\x1b[0m: No symbols to export\n");
        return 0;
    }
    if (strcmp(argv[1], "unasm") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Expected 2 arguments but got %d\n", argc-1);
            usage(argv[0]);
            return 2;
        }
        size_t bin_size;
        char *bin_data = (char *)read_file(argv[2], &bin_size);
        if (bin_data == NULL) {
            if (bin_size == 0) { // Command line argument related issue
                fprintf(stderr, "Failed to open binary file: \"%s\"\n", argv[2]);
                return 2;
            }
            // System related issue
            fprintf(stderr, "Failed to read binary file: \"%s\"\n", argv[2]);
            return 1;
        }
        Byte_View bin = (Byte_View) {
            .data = bin_data,
            .size = bin_size,
        };
        String_Builder out = {0};
        int retcode = unassemble(bin, &out);
        if (retcode != 0) return retcode;
        printf("%.*s", out.count, out.items);
        return 0;
    }
    fprintf(stderr, "Unknown command `%s'\n", argv[1]);
}
