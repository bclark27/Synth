#ifndef COMMON_H_
#define COMMON_H_

#define TRACK_MEM

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <math.h>
#include "Mem.h"

///////////////
//  DEFINES  //
///////////////

#define U1 unsigned char
#define U2 unsigned short
#define U4 unsigned int
#define U8 unsigned long

#define I1 char
#define I2 short
#define I4 int
#define I8 long

#define R4 float
#define R8 double
#define R16 long double

#define RAND_POS_DOUBLE           (rand() / (double)RAND_MAX)
#define RAND_DOUBLE               ((RAND_POS_DOUBLE * 2) - 1)
#define MIN(a,b)                  (((a)<(b))?(a):(b))
#define MAX(a,b)                  (((a)>(b))?(a):(b))
#define MAP(i1, i2, o1, o2, x)    ((o1) + (R8)(((o2) - (o1)) / (R8)((i2) - (i1))) * (R8)((x) - (i1)))
#define INTERP(a, b, steps, x)    ((R8)(a) + (((R8)(b) - (R8)(a)) / (R8)(steps)) * (R8)(x))

#define PI        3.14159265358979323846f
#define PI2       (PI * 2)
#define PI_ON_TWO (PI / 2)
#define TWO_ON_PI (2 / PI)

#define BIT_0     (1 << 0)
#define BIT_1     (1 << 1)
#define BIT_2     (1 << 2)
#define BIT_3     (1 << 3)
#define BIT_4     (1 << 4)
#define BIT_5     (1 << 5)
#define BIT_6     (1 << 6)
#define BIT_7     (1 << 7)
#define BIT_8     (1 << 8)
#define BIT_9     (1 << 9)
#define BIT_10    (1 << 10)
#define BIT_11    (1 << 11)
#define BIT_12    (1 << 12)
#define BIT_13    (1 << 13)
#define BIT_14    (1 << 14)
#define BIT_15    (1 << 15)
#define BIT_16    (1 << 16)
#define BIT_17    (1 << 17)
#define BIT_18    (1 << 18)
#define BIT_19    (1 << 19)
#define BIT_20    (1 << 20)
#define BIT_21    (1 << 21)
#define BIT_22    (1 << 22)
#define BIT_23    (1 << 23)
#define BIT_24    (1 << 24)
#define BIT_25    (1 << 25)
#define BIT_26    (1 << 26)
#define BIT_27    (1 << 27)
#define BIT_28    (1 << 28)
#define BIT_29    (1 << 29)
#define BIT_30    (1 << 30)
#define BIT_31    (1 << 31)

#define BIT_SIZE(value)                 (sizeof(value) * 8)
#define BIT_GEN_MASK(bit)               (1 << (bit))
#define BIT_MASK(value, mask)           ((value) & (mask))
#define BIT_GET(value, bit)             ((value) & BIT_GEN_MASK(bit))
#define BIT_SET(value, bit)             ((value) | BIT_GEN_MASK(bit))
#define BIT_CLEAR(value, bit)           ((value) & ~BIT_GEN_MASK(bit))
#define BIT_FLIP(value, bit)            ((value) ^ BIT_GEN_MASK(bit))

/////////////
//  TYPES  //
/////////////

typedef void (* FreeDataFunction)(void *);
typedef U1 (* CompareFunction)(void *, void *);
typedef void ( *callbackFunction)(void *, void *);
typedef U4 ( *HashFunction)(void *, U4);

/////////////////////////////
//  FUNCTION DECLERATIONS  //
/////////////////////////////

#endif
