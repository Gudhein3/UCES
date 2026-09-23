#ifndef _DEV_INCLUDE
#define _DEV_INCLUDE
#include "uces.h"

// "Do the dumb thing and the smart one will became obvious."
typedef void(*DevFuncPoke8)(u32 addr, u8 value);
typedef void(*DevFuncPeek8)(u32 addr, u8 *result);
typedef void(*DevFuncPoke32)(u32 addr, u32 value);
typedef void(*DevFuncPeek32)(u32 addr, u32 *result);

typedef struct {
    // If your device doesn't require separated 8 bit and 32 bit access, just write only the required group of them and NULL the other
    DevFuncPoke8 poke8;
    DevFuncPeek8 peek8;
    DevFuncPoke32 poke32;
    DevFuncPeek32 peek32;
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
