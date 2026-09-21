#include "mmu.h"
#include "dev.h"
#include "cpu.h"


u8 PEEK8(u32 addr) {
    for (size_t i = 0; i < sizeof(devices)/sizeof(*devices); ++i) {
        if (!devices[i]->_mapped || addr < devices[i]->_mapping_from \
            || addr+3 > devices[i]->_mapping_to)
            continue;
        if (devices[i]->peek8) {
            u8 b;
            devices[i]->peek8(addr-devices[i]->_mapping_from, &b);
            return b;
        }
        else if (devices[i]->peek32) {
            u32 b;
            devices[i]->peek32(addr-devices[i]->_mapping_from, &b);
            return b;
        }
        else {
            fprintf(stderr, "Something is wrong with the device %08X\n", devices[i]->udid);
            abort();
        }
    }
    fault("Memory address %08X doesn't belong to any device", addr);
}

u16 PEEK16(u32 addr) {
    if (addr+2 < addr) // Avoid overflows.
        fault("Invalid memory address: %08X: Overflow", addr);
    return PEEK8(addr) | (PEEK8(addr + 1) << 8);
}

u32 PEEK32(u32 addr) {
    if (addr+3 < addr) // Avoid overflows.
        fault("Invalid memory address: %08X: Overflow", addr);
    for (size_t i = 0; i < sizeof(devices)/sizeof(*devices); ++i) {
        if (!devices[i]->_mapped || addr < devices[i]->_mapping_from \
            || addr+3 > devices[i]->_mapping_to)
            continue;
        if (devices[i]->peek32) {
            u32 b;
            devices[i]->peek32(addr-devices[i]->_mapping_from, &b);
            return b;
        }
        else if (devices[i]->peek8) {
            u8 b;
            u32 c = 0;
            devices[i]->peek8(addr - devices[i]->_mapping_from, &b);
            c |= b;
            devices[i]->peek8(addr + 1 - devices[i]->_mapping_from, &b);
            c |= b << 8;
            devices[i]->peek8(addr + 2 - devices[i]->_mapping_from, &b);
            c |= b << 16;
            devices[i]->peek8(addr + 3 - devices[i]->_mapping_from, &b);
            c |= b << 24;
            return b;
        }
        else {
            fprintf(stderr, "Something is wrong with the device %08X\n", devices[i]->udid);
            abort();
        }
    }
    fault("Memory address %08X doesn't belong to any device", addr);
}

u32 PEEKS8(u32 addr) {
    return (u32)(s32)(s8)PEEK8(addr);
}

u32 PEEKS16(u32 addr) {
    return (u32)(s32)(s16)PEEK16(addr);
}

void POKE32(u32 addr, u32 b);

void POKE8(u32 addr, u8 b) {
    for (size_t i = 0; i < sizeof(devices)/sizeof(*devices); ++i) {
        if (!devices[i]->_mapped || addr < devices[i]->_mapping_from \
            || addr+3 > devices[i]->_mapping_to)
            continue;
        if (devices[i]->peek8) {
            devices[i]->poke8(addr-devices[i]->_mapping_from, b);
        }
        else if (devices[i]->peek32) {
            u32 c;
            devices[i]->peek32(addr-devices[i]->_mapping_from, &c);
            c = c & ~0xFF | b;
            devices[i]->poke32(addr-devices[i]->_mapping_from, c);
        }
        else {
            fprintf(stderr, "Something is wrong with the device %08X\n", devices[i]->udid);
            abort();
        }
        return;
    }
    fault("Memory address %08X doesn't belong to any device", addr);
}

void POKE16(u32 addr, u16 b) {
    if (addr+2 < addr) // Avoid overflows.
        fault("Invalid memory address: %08X: Overflow", addr);
    POKE8(addr + 1, b & 0xFF);
    POKE8(addr + 2, (b >> 8) & 0xFF);
}

void POKE32(u32 addr, u32 b) {
    if (addr+3 < addr) // Avoid overflows.
        fault("Invalid memory address: %08X: Overflow", addr);
    for (size_t i = 0; i < sizeof(devices)/sizeof(*devices); ++i) {
        if (!devices[i]->_mapped || addr < devices[i]->_mapping_from \
            || addr+3 > devices[i]->_mapping_to)
            continue;
        if (devices[i]->poke32) {
            devices[i]->poke32(addr - devices[i]->_mapping_from, b);
        }
        else if (devices[i]->poke8) {
            devices[i]->poke8(addr - devices[i]->_mapping_from, b & 0xFF);
            devices[i]->poke8(addr + 1 - devices[i]->_mapping_from, (b >> 8) & 0xFF);
            devices[i]->poke8(addr + 2 - devices[i]->_mapping_from, (b >> 16) & 0xFF);
            devices[i]->poke8(addr + 3 - devices[i]->_mapping_from, (b >> 24) & 0xFF);
        }
        else {
            fprintf(stderr, "Something is wrong with the device %08X\n", devices[i]->udid);
            abort();
        }
        return;
    }
    fault("Memory address %08X doesn't belong to any device", addr);
}
