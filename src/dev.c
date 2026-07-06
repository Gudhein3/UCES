#include "uces.h"

static int uart_poke(u32 addr, u8 value) {
    if (addr == 0x7FE00000) {
        putchar(value);
        fflush(stdout);
        return 1;
    }
    return 0;
}

static int uart_peek(u32 addr, u8 *result) {
    if (addr == 0x7FE00000) {
        fflush(stdin);
        result = getchar();
        return 1;
    }
    return 0;
}

static int drive_poke(u32 addr, u8 value) {
    return 0;
}

static int drive_peek(u32 addr, u8 *result) {
    return 0;
}

Dev uart_device = {
    uart_poke,
    uart_peek,
};
