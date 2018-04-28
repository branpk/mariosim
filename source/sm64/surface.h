#ifndef SURFACE_H
#define SURFACE_H

#include "types.h"

struct Surface
{
    /*0x00*/ s16 type;
    /*0x02*/ s16 unk02;
    /*0x04*/ s8 flags;
    /*0x05*/ s8 unk05;
    /*0x06*/ s16 lowerY;
    /*0x08*/ s16 upperY;
    /*0x0A*/ Vec3s vertex1;
    /*0x10*/ Vec3s vertex2;
    /*0x16*/ Vec3s vertex3;
    /*0x1C*/ Vec3f normal;
    /*0x28*/ f32 originOffset;
    /*0x2C*/ u32 /*struct Object **/object;
};

#endif