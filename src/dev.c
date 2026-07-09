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
        *result = getchar();
        return 1;
    }
    return 0;
}

static FILE *drive_file;
static size_t drive_sec = 0;
static size_t drive_off = 0;

static int drive_poke(u32 addr, u8 value) {
    if (addr == 0x7FE00010)
        drive_sec = drive_sec&~0x000000FF|(addr&0xFF<< 0);
    else if (addr == 0x7FE00011)
        drive_sec = drive_sec&~0x0000FF00|(addr&0xFF<< 8);
    else if (addr == 0x7FE00012)
        drive_sec = drive_sec&~0x00FF0000|(addr&0xFF<<16);
    else if (addr == 0x7FE00013)
        drive_sec = drive_sec&~0xFF000000|(addr&0xFF<<24);
    else if (addr == 0x7FE00014)
        return 0;
    else if (addr == 0x7FE00015) {
        fseek(drive_file, drive_sec*512+drive_off, SEEK_SET);
        fwrite(&value, 1, 1, drive_file);
        drive_off++;
        drive_sec += drive_off/512;
        drive_off %= 512;
        // TODO: get status
    }
    if (addr >= 0x7FE00010 && addr <= 0x7FE00020)
        return 1;
    return 0;
}

static int drive_peek(u32 addr, u8 *result) {
    if (addr == 0x7FE00010)
        *result = (drive_sec>>0)&0xFF;
    else if (addr == 0x7FE00011)
        *result = (drive_sec>>8)&0xFF;
    else if (addr == 0x7FE00012)
        *result = (drive_sec>>16)&0xFF;
    else if (addr == 0x7FE00013)
        *result = (drive_sec>>24)&0xFF;
    else if (addr == 0x7FE00014)
        *result = 69;
    else if (addr == 0x7FE00015) {
        fseek(drive_file, drive_sec*512+drive_off, SEEK_SET);
        fread(result, 1, 1, drive_file);
        drive_off++;
        drive_sec += drive_off/512;
        drive_off %= 512;
        // TODO: get status
    }
    if (addr >= 0x7FE00010 && addr <= 0x7FE00020)
        return 1;
    return 0;
}

Dev uart_device = {
    uart_poke,
    uart_peek,
};

Dev drive_device = {
    drive_poke,
    drive_peek,
};

int init_dev() {
    drive_file = fopen("drive.bin", "r+");
    if (!drive_file) {
        perror("Failed to open drive file");
        return 1;
    }
    return 0;
}
