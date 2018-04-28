#ifndef TYPES_H
#define TYPES_H


#include <stdint.h>


#define TRUE 1
#define FALSE 0

typedef int8_t   s8;
typedef uint8_t  u8;
typedef int16_t  s16;
typedef uint16_t u16;
typedef int32_t  s32;
typedef uint32_t u32;
typedef int64_t  s64;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;


typedef f32 Vec3f[3];
typedef s16 Vec3s[3];
typedef s32 Vec3i[3];


extern u32 sineTableRaw[];
#define sinTable ((f32 *) &sineTableRaw[0])
#define cosTable ((f32 *) &sineTableRaw[1024])
#define sins(x) sinTable[(u16) (x) >> 4]
#define coss(x) cosTable[(u16) (x) >> 4]

extern s16 atanTable[];
s16 atan2xy(f32 x, f32 y);


#define UNUSED __attribute__((unused))


#endif