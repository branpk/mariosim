
#include "sm64/types.h"

s16 atan2oct(f32 y, f32 x) {
  if (x == 0.0f)
    return atanTable[0];
  return atanTable[(s32) ((y / x) * 1024.0f + 0.5f)];
}

s16 atan2xy(f32 x, f32 y) {
  if (y >= 0.0f) {
    if (x >= 0.0f) {
      if (x >= y)
        return atan2oct(y, x);
      else 
        return 0x4000 - atan2oct(x, y);
    }
    else {
      x = -x;
      if (x < y)
        return atan2oct(x, y) + 0x4000;
      else 
        return 0x8000 - atan2oct(y, x);
    }
  }
  else {
    y = -y;
    if (x < 0.0f) {
      x = -x;
      if (x >= y)
        return atan2oct(y, x) + 0x8000;
      else {
        return 0xC000 - atan2oct(x, y);
      }
    }
    else {
      if (x < y)
        return atan2oct(x, y) + 0xC000;
      else
        return -atan2oct(y, x);
    }
  }
}
