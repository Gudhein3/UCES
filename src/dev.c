#include "dev.h"
#include "cpu.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


static void dmem_poke8(u32 addr, u8 value) {
    if (DBG >= 2)
        fprintf(stderr, "[DEBUG] MWrite8 %02X -> %08X\n", value, addr);
    if (addr < vm.memory_size) {
        vm.memory[addr] = value;
    }
}

static void dmem_peek8(u32 addr, u8 *result) {
    if (addr < vm.memory_size) {
        *result = vm.memory[addr];
    }
    else {
        *result = 0;
    }
    if (DBG >= 2)
        fprintf(stderr, "[DEBUG] MRead8 %08X -> %02X\n", addr, *result);
}

static void dmem_poke32(u32 addr, u32 value) {
    if (DBG >= 2)
        fprintf(stderr, "[DEBUG] MWrite32 %08X -> %08X\n", value, addr);
    if (addr < vm.memory_size) {
        *(u32 *)&vm.memory[addr] = value;
    }
}

static void dmem_peek32(u32 addr, u32 *result) {
    if (addr < vm.memory_size) {
        *result = *(u32 *)&vm.memory[addr];
    }
    else {
        *result = 0;
    }
    if (DBG >= 2)
        fprintf(stderr, "[DEBUG] MRead32 %08X -> %08X\n", addr, *result);
}

static Dev dmem_device = {
    dmem_poke8,
    dmem_peek8,
    dmem_poke32,
    dmem_peek32,
    0x00000000,
    0x00000000,
    0xFFFFFFFF,
    1
};

static void uart_poke8(u32 addr, u8 value) {
    if (addr == 0) {
        putchar(value);
        fflush(stdout);
    }
}

static void uart_peek8(u32 addr, u8 *result) {
    if (addr == 0) {
        fflush(stdin);
        *result = getchar();
    }
}

static FILE *drive_file = NULL;
static size_t drive_size = 0;

// It's techinically should be called a "file" device.
static void drive_poke32(u32 addr, u32 value) {
    if (addr == 0) {
        if (drive_file) fclose(drive_file);
        drive_file = fopen(&vm.memory[value], "rb");
    }
    else if (addr == 4) {
        if (drive_file) fclose(drive_file);
        drive_file = fopen(&vm.memory[value], "wb");
    }
    else if (addr == 8) {
        if (drive_file) fclose(drive_file);
        drive_file = NULL;
    }
    else if (addr == 12) {
        drive_size = value;
    }
    else if (addr == 16) {
        fread(&vm.memory[value], 1, drive_size, drive_file);
    }
    else if (addr == 20) {
        fwrite(&vm.memory[value], 1, drive_size, drive_file);
    }
    else if (addr == 24) {
        fseek(drive_file, value, SEEK_SET);
    }
}

static void drive_peek32(u32 addr, u32 *result) {
    if (!drive_file) {
        *result = 0;
        return;
    }
    if (addr == 24) {
        *result = ftell(drive_file);
    }
    else if (addr == 28) {
        size_t c = ftell(drive_file);
        fseek(drive_file, 0, SEEK_END);
        *result = ftell(drive_file);
        fseek(drive_file, c, SEEK_SET);
    }
}

extern bool is_screen_initialized;
extern SDL_Renderer *screen_ren;
extern SDL_Window *screen_win;
extern SDL_Texture *screen_tex;
extern u32 screen_bufa[16 * 16];
extern u32 screen_bufb[16 * 16];
extern int screen_bufx, screen_bufy;
extern int screen_command;

#define SDL_WRAP(msg, ...) do { \
    if ((__VA_ARGS__)) {\
        fprintf(stderr, msg, SDL_GetError());\
        exit(1); \
    }} while(0)

static void scr_poke32(u32 addr, u32 value) {
    if (addr == 0 && !is_screen_initialized) {
        u16 w = value >> 16;
        u16 h = value;
        SDL_WRAP("Failed to initialize SDL: %s\n", SDL_Init(SDL_INIT_VIDEO) < 0);

        screen_win = SDL_CreateWindow("UCES", w, h, 0);
        SDL_WRAP("Failed to open %d x %d window: %s\n", w, h, !screen_win);

        screen_ren = SDL_CreateRenderer(screen_win, NULL);

        SDL_WRAP("Failed to create renderer: %s\n", !screen_ren);

        screen_tex = SDL_CreateTexture(screen_ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h);

        SDL_WRAP("Failed to create texture: %s\n", !screen_tex);
        is_screen_initialized = true;
    }
    else if (addr == 4) {
        u16 x = value >> 16;
        u16 y = value;
        screen_bufx = x;
        screen_bufy = y;
        memcpy(screen_bufb, screen_bufa, 16 * 16 * 4);
        screen_command = 1;
    }
    else if (addr == 8) {
        for (int i = 0; i < 16 * 16; ++i) {
            screen_bufa[i] = value;
        }
    }
    else if (addr == 16) {
        memcpy(screen_bufa, &vm.memory[value], 16 * 16 * 4);
    }
}

static void scr_peek32(u32 addr, u32 *result) {

}

static Dev uart_device = {
    uart_poke8,
    uart_peek8,
    NULL, NULL,
    0x1F822D0C,
    0x7FE00000,
    0x7FE0000F,
    1
};

static Dev drive_device = {
    NULL, NULL,
    drive_poke32,
    drive_peek32,
    0xE2DECD6B,
    0x7FE00010,
    0x7FE00030,
    1
};

static Dev scr_device = {
    NULL, NULL,
    scr_poke32,
    scr_peek32,
    0x158BF2AD,
    0x7FE00040,
    0x7FE00500,
    1
};

Dev *devices[] = {
    &uart_device,
    &drive_device,
    &scr_device,
    &dmem_device
};

int init_dev() {
    return 0;
}
