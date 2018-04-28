#ifndef MARIO_H
#define MARIO_H

#include "sm64/input.h"
#include "sm64/surface.h"
#include "sm64/types.h"

struct MarioState
{
    // /*0x00*/ u16 unk00;
    // /*0x02*/ u16 input;
    // /*0x04*/ u32 flags;
    // /*0x08*/ u32 unk08;
    // /*0x0C*/ u32 action;
    // /*0x10*/ u32 prevAction;
    // /*0x14*/ u32 unk14;
    // /*0x18*/ u16 actionState;
    // /*0x1A*/ u16 actionTimer;
    // /*0x1C*/ u32 actionArg;
    /*0x20*/ f32 intendedMag;
    /*0x24*/ s16 intendedYaw;
    // /*0x26*/ s16 invincTimer;
    // /*0x28*/ u8 framesSinceA;
    // /*0x29*/ u8 framesSinceB;
    // /*0x2A*/ u8 wallKickTimer;
    // /*0x2B*/ u8 doubleJumpTimer;
    /*0x2C*/ Vec3s faceAngle;
    // /*0x32*/ Vec3s angleVel;
    /*0x38*/ s16 slideYaw;
    /*0x3A*/ s16 twirlYaw;
    // /*0x3C*/ Vec3f pos;
    // /*0x48*/ Vec3f vel;
    /*0x54*/ f32 forwardVel;
    /*0x58*/ f32 slideVelX;
    /*0x5C*/ f32 slideVelZ;
    // /*0x60*/ struct Surface *wall;
    // /*0x64*/ struct Surface *ceil;
    /*0x68*/ struct Surface *floor;
    // /*0x6C*/ f32 ceilHeight;
    // /*0x70*/ f32 floorHeight;
    // /*0x74*/ s16 floorAngle;
    // /*0x76*/ s16 waterLevel;
    // /*0x78*/ u32 /*struct Object **/interactObj;
    // /*0x7C*/ u32 /*struct Object **/heldObj;
    // /*0x80*/ u32 /*struct Object **/usedObj;
    // /*0x84*/ u32 /*struct Object **/riddenObj;
    // /*0x88*/ u32 /*struct Object **/marioObj;
    // /*0x8C*/ u32 level;
    // /*0x90*/ u32 /*struct Area **/area;
    // /*0x94*/ u32 /*struct UnknownStruct6 **/unk94;
    // /*0x98*/ u32 /*struct UnknownStruct4 **/unk98;
    /*0x9C*/ struct Controller *controller;
    // /*0xA0*/ u32 animation;
    // /*0xA4*/ u32 collidedObjInteractTypes;
    // /*0xA8*/ s16 numCoins;
    // /*0xAA*/ s16 numStars;
    // /*0xAC*/ u8 unkAC;
    // /*0xAD*/ s8 numLives;
    // /*0xAE*/ s16 health;
    // /*0xB0*/ s16 unkB0;
    // /*0xB2*/ u8 hurtCounter;
    // /*0xB3*/ u8 healCounter;
    // /*0xB4*/ u8 squishTimer;
    // /*0xB5*/ u8 unkB5;
    // /*0xB6*/ u16 capTimer;
    // /*0xB8*/ s16 unkB8;
    // /*0xBC*/ f32 peakHeight;
    // /*0xC0*/ f32 quicksandDepth;
    // /*0xC4*/ f32 unkC4;
};


u32 actCrouchSliding(struct MarioState *m);
void setMarioInputAnalog(struct MarioState *m, s16 cameraAngle);


#endif