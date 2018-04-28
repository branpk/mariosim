#include "goal.h"
#include "ol.h"
#include "sm64/input.h"
#include "sm64/mario.h"
#include "sm64/surface.h"
#include "sm64/types.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Goal optGoal;

struct Controller controller;
struct Surface marioFloor;
struct MarioState marioState;
s16 cameraAngle;

struct MarioState endMarioState;


void performStep(s8 rawStickX, s8 rawStickY) {
  memcpy(&endMarioState, &marioState, sizeof(struct MarioState));

  endMarioState.controller = &controller;
  endMarioState.floor = &marioFloor;

  controller.rawStickX = rawStickX;
  controller.rawStickY = rawStickY;
  adjust_analog_stick(&controller);

  setMarioInputAnalog(&endMarioState, cameraAngle);
  actCrouchSliding(&endMarioState);
}


int goalReached = 0;
struct MarioState bestMarioState;
s8 bestStickX;
s8 bestStickY;


int checkGoal(struct MarioState *m) {
  switch (optGoal.type) {

  case goal_exact:
    return state_value(m, optGoal.value, f32) == optGoal.exact.target;

  case goal_max:
    if (!goalReached) return 1;
    return state_value(m, optGoal.value, f32) >
      state_value(&bestMarioState, optGoal.value, f32);

  case goal_maxlt:
    if (state_value(m, optGoal.value, f32) >= optGoal.maxlt.target)
      return 0;
    if (!goalReached) return 1;
    return state_value(m, optGoal.value, f32) >
      state_value(&bestMarioState, optGoal.value, f32);

  case goal_near:
    if (!goalReached) return 1;
    return fabs(state_value(m, optGoal.value, f32) - optGoal.near.target) <
      fabs(state_value(&bestMarioState, optGoal.value, f32) - optGoal.near.target);
  }
}


void solveGoal(void) {
  for (s32 x = -128; x <= 127; x++) {
    for (s32 y = -128; y <= 127; y++) {
      performStep((s8) x, (s8) y);

      if (checkGoal(&endMarioState)) {
        goalReached = 1;
        memcpy(&bestMarioState, &endMarioState, sizeof(struct MarioState));
        bestStickX = (s8) x;
        bestStickY = (s8) y;
      }
    }
  }
}


static void error(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  fprintf(stderr, "\x1b[91mError:\x1b[0m ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  
  va_end(args);
  exit(1);
}


void printState(void) {
  struct MarioState *m = &marioState;
  struct Surface *floor = &marioFloor;

  printf("mario:\n");
  printf("  facing yaw = 0x%hX\n", m->faceAngle[1]);
  printf("  sliding yaw = 0x%hX\n", m->slideYaw);
  printf("  h speed = %f\n", m->forwardVel);
  printf("  sliding speed = (%f, %f)\n", m->slideVelX, m->slideVelZ);

  printf("floor:\n");
  printf("  type = 0x%X\n", floor->type);
  printf("  normal = (%f, %f, %f)\n", floor->normal[0], floor->normal[1], floor->normal[2]);

  printf("camera:\n");
  printf("  angle = 0x%hX\n", cameraAngle);
}


void loadMarioState(OlBlock *in, struct MarioState *m) {
  char *validFields[] = {
    "facingyaw",
    "slidingyaw",
    "hspeed",
    "slidingspeed",
    NULL
  };
  ol_checkNoInvalidFields(in, validFields);

  m->faceAngle[1] = ol_checkFieldInt(in, "facingyaw");
  m->slideYaw = ol_checkFieldInt(in, "slidingyaw");
  m->forwardVel = ol_checkFieldFloat(in, "hspeed");

  char *xzFields[] = {"x", "z", NULL};
  OlBlock *slidingSpeed = ol_createFieldNamedArray(in, "slidingspeed", xzFields);
  m->slideVelX = ol_checkFieldFloat(slidingSpeed, "x");
  m->slideVelZ = ol_checkFieldFloat(slidingSpeed, "z");
  ol_freeNamedArray(slidingSpeed);
}


void loadFloorState(OlBlock *in, struct Surface *floor) {
  char *validFields[] = {
    "type",
    "normal",
    NULL
  };
  ol_checkNoInvalidFields(in, validFields);

  floor->type = ol_checkFieldInt(in, "type");

  char *xyzFields[] = {"x", "y", "z", NULL};
  OlBlock *normalArray = ol_createFieldNamedArray(in, "normal", xyzFields);
  floor->normal[0] = ol_checkFieldFloat(normalArray, "x");
  floor->normal[1] = ol_checkFieldFloat(normalArray, "y");
  floor->normal[2] = ol_checkFieldFloat(normalArray, "z");
  ol_freeNamedArray(normalArray);
}


void loadCameraState(OlBlock *in, s16 *cameraAngle) {
  char *validFields[] = {
    "angle",
    NULL
  };
  ol_checkNoInvalidFields(in, validFields);

  *cameraAngle = ol_checkFieldInt(in, "angle");
}


void loadOptGoal(OlValue *in) {
  printf("\x1b[1mGoal:\x1b[0m\n");

  if (!ol_isIdent(in->call.func, "solve"))
    error("Cannot call '%s' here\n", ol_valueStr(in->call.func));
  ol_checkNumArgs(in, 1, 1);

  char *validGoalValues[] = {
    "hspeed",
    NULL
  };
  ol_checkNoInvalidFields(in->call.args, validGoalValues);

  OlValue *g = NULL;

  if ((g = ol_getField(in->call.args, "hspeed")) != NULL) {
    printf("Optimizing for H speed\n");
    optGoal.value = goal_value(forwardVel);
  }

  if (g == NULL)
    error("Invalid optimization goal: %s", ol_valueStr(in));

  int validGoal = 0;

  if (g->type == ol_call) {
    if (ol_isIdent(g->call.func, "max")) {
      ol_checkNumArgs(g, 0, 0);
      printf("Maximizing value\n");
      optGoal.type = goal_max;
      validGoal = 1;
    }
    else if (ol_isIdent(g->call.func, "maxlt")) {
      ol_checkNumArgs(g, 1, 0);
      optGoal.type = goal_maxlt;
      optGoal.maxlt.target = ol_checkArgFloat(g, 0);
      printf("Maximizing value while staying less than %f\n", optGoal.maxlt.target);
      validGoal = 1;
    }
    else if (ol_isIdent(g->call.func, "near")) {
      ol_checkNumArgs(g, 1, 0);
      optGoal.type = goal_near;
      optGoal.near.target = ol_checkArgFloat(g, 0);
      printf("Getting value as close to %f as possible\n", optGoal.near.target);
      validGoal = 1;
    }
  }
  else {
    optGoal.type = goal_exact;
    optGoal.exact.target = ol_checkArgFloat(in, 0);
    printf("Getting an exact value of %f\n", optGoal.exact.target);
    validGoal = 1;
  }

  if (!validGoal)
    error("Invalid optimization goal: %s", ol_valueStr(g));
}


void loadState(OlBlock *in) {
  char *validFields[] = {
    "mario",
    "floor",
    "camera",
    "input",
    NULL
  };
  ol_checkNoInvalidFields(in, validFields);

  loadMarioState(ol_checkField(in, "mario", ol_block)->block, &marioState);
  loadFloorState(ol_checkField(in, "floor", ol_block)->block, &marioFloor);
  loadCameraState(ol_checkField(in, "camera", ol_block)->block, &cameraAngle);

  printf("\x1b[1mStarting state:\x1b[0m\n");
  printState();
  printf("\n");

  loadOptGoal(ol_checkField(in, "input", ol_call));
  printf("\n");
}


#if defined(WIN32)
#include <stdio.h>
#include <windows.h>
void win_enable_ansi(void) {
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  dwMode |= 4;
  SetConsoleMode(hOut, dwMode);
}
#endif


int main(int argc, char **argv) {
#if defined(WIN32)
  win_enable_ansi();
#endif

  if (argc < 2)
    error("Expected filename");

  OlBlock *file = ol_parseFile(argv[1]);
  loadState(file);
  ol_free(file);

  solveGoal();
  if (!goalReached) {
    printf("\x1b[31mFailed to reach goal\x1b[0m\n");
  }
  else {
    memcpy(&marioState, &bestMarioState, sizeof(struct MarioState));
    printf("\x1b[1mEnding state:\x1b[0m\n");
    printState();
    printf("\n");
    printf("\x1b[32mReached goal:\x1b[0m x=%d, y=%d\n", bestStickX, bestStickY);
  }

  return 0;
}
