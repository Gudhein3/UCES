#include <stdbool.h>
#include "uces.h"
#include "libasm.h"
#include "cpu.h"
#include "dev.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

bool is_screen_initialized = false;
SDL_Renderer *screen_ren;
SDL_Window *screen_win;
SDL_Texture *screen_tex;
u32 screen_bufa[16 * 16] = {0};
u32 screen_bufb[16 * 16] = {0};
int screen_bufx = 0, screen_bufy = 0;
int screen_command = 0;
VM vm;

void usage(char *prog) {
    fprintf(stderr, "usage: %s <binary UCES executable>\n", prog);
}

int main(int argc, char **argv) {
    s32 botest;
    memcpy(&botest, "ABCD", 4);
    if (botest != 0x44434241) {
        fprintf(stderr, "Sorry, this software doesn't support big-endian systems.\n");
        return 1;
    }
    if (argc != 2) {
        usage(argv[0]);
        fprintf(stderr, "Expected 1 argument but got %d\n", argc-1);
        return 2;
    }
    SDL_Event event;
    size_t program_size;
    u8 *program = read_file(argv[1], &program_size);
    if (!program) panic("Failed to read program.\n", NULL);
    if (program_size > MEMSIZE) panic("Program too big:\n\tProgram size is {"CSize_Fmt"}, while Memory size is {"CSize_Fmt"}", CSIZE_FORMAT(program_size), CSIZE_FORMAT(MEMSIZE));
    vm.memory = calloc(1, MEMSIZE);
    vm.memory_size = MEMSIZE;
    vm.registers[1] = STACK_START;
    memcpy(vm.memory, program, program_size);
    free(program);
    int status;
    String_Builder inst_deasm = {0};
    if (init_dev() != 0) {
        fprintf(stderr, "Failed to initialize virtual devices :(\n");
        return 1;
    }
    for (;;) {
        if (is_screen_initialized) {
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_EVENT_QUIT: goto halt;
                    case SDL_EVENT_KEY_DOWN: break;
                    case SDL_EVENT_MOUSE_MOTION:
                        break;
                }
            }
            SDL_SetRenderDrawColor(screen_ren, 0, 0, 0, 255);
            SDL_RenderClear(screen_ren);
            if (screen_command) {
                SDL_Rect rect = (SDL_Rect) {screen_bufx, screen_bufy, 16, 16};
                SDL_UpdateTexture(screen_tex, &rect, screen_bufb, 16 * 4);
                screen_command = 0;
            }
            SDL_RenderTexture(screen_ren, screen_tex, NULL, NULL);
            SDL_RenderPresent(screen_ren);
        }
        for (int i = 0; i < 128; ++i) {
            if (DBG >= 3) {
                inst_deasm.count = 0;
                unassemble((Byte_View) {vm.memory+vm.pc, 4}, &inst_deasm);
                fprintf(stderr, "[DEBUG] Inst: 0x%08X: %.*s", vm.pc, inst_deasm.count, inst_deasm.items);
            }
            status = cpu_tick();
            if (DBG >= 4) {
                fprintf(stderr, "[DEBUG] Status: %d\n", status);
                fprintf(stderr, "[DEBUG] Registers:\n");
                for (int i = 0; i < 32; ++i) {
                    fprintf(stderr, "  x%-2d %-4s: 0x%08X\n", i, regnames[i], read_register(i));
                }
            }
            if (DBG >= 5) {
                fprintf(stderr, "[DEBUG] Press Enter to continue\n");
                getchar();
            }
            if (status & EIS_HALT) goto halt;
            if ((status & EIS_OVPC) == 0) vm.pc += 4;
        }
    }
    halt:
}
