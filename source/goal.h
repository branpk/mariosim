#ifndef GOAL_H
#define GOAL_H

#include "sm64/mario.h"
#include "sm64/types.h"

#include <stddef.h>
#include <stdlib.h>

typedef enum {
  goal_exact,
  goal_max,
  goal_maxlt,
  goal_near,
} GoalType;

typedef struct {
  GoalType type;
  union {
    struct { f32 target; } exact;
    struct { f32 target; } maxlt;
    struct { f32 target; } near;
  };
  size_t value;
} Goal;

#define goal_value(x) offsetof(struct MarioState, x)
#define state_value(m, v, t) *((t *) ((u8 *) (m) + v))

#endif