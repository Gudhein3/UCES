#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _Nullable

// You can specify DBG when building via compiler flag "-DDBG={XXX}"
#ifndef DBG
#define DBG 1
#endif

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef  int8_t  s8;
typedef  int16_t s16;
typedef  int32_t s32;
typedef  int64_t s64;

typedef float f32;
typedef double f64;

#define __STR0(x) #x
#define __STR(x) __STR0(x)

#define KIB(k, b) (k)*1024+(b)
#define MIB(m, b) KIB((m)*1024,b)
#define GIB(g, b) MIB((g)*1024,b)
#define MEMSIZE MIB(12, 0)
#define STACK_START (MEMSIZE-MIB(1, 0))

#define CSize_Fmt "%uGiB%uMib%uKiB%uB"

#define CSIZE_FORMAT(c) ((c)/1024/1024/1024), (((c)/1024/1024)%1024), (((c)/1024)%1024), ((c)%1024)

u8 *read_file(const char *filename, size_t *_Nullable size);
