#ifndef _DA_INCLUDE
#define _DA_INCLUDE
#include <stdlib.h>
#include <string.h>

#define DA_INITIAL_CAPACITY 256

#define da_append(xs, x)                                                             \
    do {                                                                             \
        if ((xs)->count >= (xs)->capacity) {                                         \
            if ((xs)->capacity == 0) (xs)->capacity = DA_INITIAL_CAPACITY;           \
            else (xs)->capacity += (xs)->capacity>>1;                                \
            (xs)->items = realloc((xs)->items, (xs)->capacity*sizeof(*(xs)->items)); \
        }                                                                            \
        (xs)->items[(xs)->count++] = (x);                                            \
    } while (0)

#define da_extend(xs, xc, x)                                                         \
    do {                                                                             \
        if ((xs)->count+(xc) >= (xs)->capacity) {                                    \
            if ((xs)->capacity == 0) (xs)->capacity = DA_INITIAL_CAPACITY;           \
            else (xs)->capacity += ((xs)->capacity>>1) + (xc);                       \
            (xs)->items = realloc((xs)->items, (xs)->capacity*sizeof(*(xs)->items)); \
        }                                                                            \
        memcpy((xs)->items+(xs)->count, (x), xc);                                    \
        (xs)->count += (xc);                                                         \
    } while (0)

#endif // _DA_INCLUDE
