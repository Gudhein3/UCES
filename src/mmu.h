#ifndef _MMU_INCLUDE
#define _MMU_INCLUDE
#include "uces.h"

u8 PEEK8(u32 addr);
u16 PEEK16(u32 addr);
u32 PEEK32(u32 addr);
u32 PEEKS8(u32 addr);
u32 PEEKS16(u32 addr);

void POKE32(u32 addr, u32 b);
void POKE8(u32 addr, u8 b);
void POKE16(u32 addr, u16 b);
void POKE32(u32 addr, u32 b);

#endif // _MMU_INCLUDE
