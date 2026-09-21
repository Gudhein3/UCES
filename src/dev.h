#ifndef _DEV_INCLUDE
#define _DEV_INCLUDE
#include "uces.h"

// "Do the dumb thing and the smart one will became obvious."
typedef void(*dev_func_poke8_t)(u32 addr, u8 value);
typedef void(*dev_func_peek8_t)(u32 addr, u8 *result);
typedef void(*dev_func_poke32_t)(u32 addr, u32 value);
typedef void(*dev_func_peek32_t)(u32 addr, u32 *result);

typedef struct {
    // If your device doesn't require separated 8 bit and 32 bit access, just write only the required group of them and NULL the other
    dev_func_poke8_t poke8;
    dev_func_peek8_t peek8;
    dev_func_poke32_t poke32;
    dev_func_peek32_t peek32;
    uint32_t udid; // Unique Device IDentifier
    // The following fields are meant to be used only by the MMU;
    // You should not touch them.
    uint32_t _mapping_from;
    uint32_t _mapping_to;
    int/*Used as a boolean*/ _mapped;
} Dev;

extern Dev *devices[4];

int init_dev();
#endif // _DEV_INCLUDE
