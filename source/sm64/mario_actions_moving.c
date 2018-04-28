#include "sm64/mario.h"

#include "sm64/types.h"

#include <math.h>
#include <stdlib.h>


s16 getFloorSlipperyClass(struct MarioState *m) {
  s16 class;
  // if ((m->area->v02 & 0x0007) == 6)
    // class = 0x0013;
  // else
    class = 0;

  if (m->floor != NULL) {
    switch (m->floor->type) {
    case 0x0073: class = 0x0013; break;
    case 0x0074: class = 0x0013; break;
    case 0x0075: class = 0x0013; break;
    case 0x0078: class = 0x0013; break;
    case 0x0079: class = 0x0014; break;
    case 0x007A: class = 0x0015; break;
    case 0x0013: class = 0x0013; break;
    case 0x0014: class = 0x0014; break;
    case 0x0015: class = 0x0015; break;
    case 0x002A: class = 0x0014; break;
    case 0x002E: class = 0x0013; break;
    case 0x0035: class = 0x0014; break;
    case 0x0036: class = 0x0013; break;
    case 0x0037: class = 0x0015; break;
    }
  }

  // if (m->action == ma_crawling && m->floor->normal[1] > 0.5f && class == 0)
  //   class = 0x0015;

  return class;
}


void p80263FFC(struct MarioState *m, f32 accel, f32 lossFactor) {
  f32 nx = m->floor->normal[0];
  f32 ny = m->floor->normal[1];
  f32 nz = m->floor->normal[2];

  f32 slopeAngle = atan2xy(nz, nx);
  f32 steepness = sqrtf(nx*nx + nz*nz);
  
  m->slideVelX += accel * steepness * sins(slopeAngle);
  m->slideVelZ += accel * steepness * coss(slopeAngle);
  
  m->slideVelX *= lossFactor;
  m->slideVelZ *= lossFactor;
  
  m->slideYaw = atan2xy(m->slideVelZ, m->slideVelX);
  
  s32 dyaw = (s16) (m->faceAngle[1] - m->slideYaw);

  //! -0x4000, 0x8000 not handled - also double check this decompilation
  if (dyaw > 0x0000 && dyaw <= 0x4000) {
    dyaw -= 0x200;
    if (dyaw < 0x0000) dyaw = 0x0000;
  }
  else if (dyaw > -0x4000 && dyaw < 0x0000) {
    dyaw += 0x200;
    if (dyaw > 0x0000) dyaw = 0x0000;
  }
  else if (dyaw > 0x4000 && dyaw < 0x8000) {
    dyaw += 0x200;
    if (dyaw > 0x8000) dyaw = 0x8000;
  }
  else if (dyaw > -0x8000 && dyaw < -0x4000) {
    dyaw -= 0x200;
    if (dyaw < -0x8000) dyaw = -0x8000;
  }

  m->faceAngle[1] = m->slideYaw + dyaw;
  
  // m->vel[0] = m->slideVelX;
  // m->vel[1] = 0.0f;
  // m->vel[2] = m->slideVelZ;
  
  // p802554AC(m);
  // p802555AC(m);
  
  //! H speed is capped a frame late (butt slide HSG)
  m->forwardVel =
    sqrtf(m->slideVelX * m->slideVelX + m->slideVelZ * m->slideVelZ);
  if (m->forwardVel > 100.0f) {
    m->slideVelX = m->slideVelX * 100.0f / m->forwardVel;
    m->slideVelZ = m->slideVelZ * 100.0f / m->forwardVel;
  }

  if (dyaw < -0x4000 || dyaw > 0x4000)
    m->forwardVel *= -1.0f;
}


u32 marioSlideAcceleration(struct MarioState *m, f32 stopSpeed) {
  s16 dyaw = m->intendedYaw - m->slideYaw;
  f32 forward = coss(dyaw);
  f32 sideward = sins(dyaw);
  
  //! 10k glitch
  if (forward < 0.0f && m->forwardVel >= 0.0f)
    forward *= 0.5f * m->forwardVel / 100.0f + 0.5f;

  f32 accel;
  f32 lossFactor;
  
  switch (getFloorSlipperyClass(m)) {
  case 0x0013:
    accel = 10.0f;
    lossFactor = m->intendedMag / 32.0f * forward * 0.02f + 0.98f;
    break;

  case 0x0014:
    accel = 8.0f;
    lossFactor = m->intendedMag / 32.0f * forward * 0.02f + 0.96f;
    break;

  case 0x0015:
    accel = 5.0f;
    lossFactor = m->intendedMag / 32.0f * forward * 0.02f + 0.92f;
    break;

  default:
    accel = 7.0f;
    lossFactor = m->intendedMag / 32.0f * forward * 0.02f + 0.92f;
    break;
  }

  f32 oldMag =
    sqrtf(m->slideVelX * m->slideVelX + m->slideVelZ * m->slideVelZ);

  m->slideVelX +=
    m->intendedMag / 32.0f * m->slideVelZ * sideward * 0.05f;
  m->slideVelZ -=
    m->intendedMag / 32.0f * m->slideVelX * sideward * 0.05f;

  f32 newMag =
    sqrtf(m->slideVelX * m->slideVelX + m->slideVelZ * m->slideVelZ);
  
  if (oldMag > 0.0f && newMag > 0.0f) {
    m->slideVelX = m->slideVelX * oldMag / newMag;
    m->slideVelZ = m->slideVelZ * oldMag / newMag;
  }

  p80263FFC(m, accel, lossFactor);

  // if (!floorIsSlope(m) && m->forwardVel * m->forwardVel < stopSpeed * stopSpeed) {
  //   setMarioforwardVel(m, 0.0f);
  //   return TRUE;
  // }
  return FALSE;
}


u32 p80267B74(
  struct MarioState *m, u32 stopAction, u32 jumpAction, u32 airAction, u32 anim)
{
  // if (m->actionTimer == 5) {
  //   if (m->input & input_a_pressed)
  //     return p80252E74(m, jumpAction, 0);
  // }
  // else {
  //   m->actionTimer += 1;
  // }

  marioSlideAcceleration(m, 4.0f);

  // if (marioSlideAcceleration(m, 4.0f))
  //   return setMarioAction(m, stopAction, 0);
  
  // p802678D4(m, stopAction, airAction, anim);
  return FALSE;
}


u32 actCrouchSliding(struct MarioState *m) {
  // if (m->input & input_above_slide)
    // return setMarioAction(m, ma_butt_slide, 0);

  // if (m->actionTimer < 30) {
  //   m->actionTimer += 1;
  //   if ((m->input & input_a_pressed) && m->forwardVel > 10.0f)
  //     return p80252E74(m, ma_long_jump, 0);
  // }

  // if (m->input & input_b_pressed) {
  //   if (m->forwardVel >= 10.0f)
  //     return setMarioAction(m, ma_slide_kick, 0);
  //   else
  //     return setMarioAction(m, ma_move_punching, 9);
  // }

  // if (m->input & input_a_pressed)
  //   return p80252E74(m, ma_jump, 0);

  // if (m->input & input_first_person)
  //   return setMarioAction(m, ma_skidding, 0);

  // return p80267B74(m, ma_crouching, ma_jump, ma_freefall, m_anim_unknown_0097);

  return p80267B74(m, 0, 0, 0, 0);
}


void setMarioInputAnalog(struct MarioState *m, s16 cameraAngle) {
  struct Controller *controller = m->controller;
  f32 squareMag = (controller->stickMag / 64.0f) * (controller->stickMag / 64.0f) * 64.0f;

  // if (m->squishTimer == 0)
    m->intendedMag = squareMag / 2.0f;
  // else
  //   m->intendedMag = squareMag / 8.0f;
  
  if (m->intendedMag > 0.0f) {
    m->intendedYaw = atan2xy(-controller->stickY, controller->stickX) + cameraAngle;
    // m->input |= input_nonzero_analog;
  }
  else {
    m->intendedYaw = m->faceAngle[1];
  }
}
