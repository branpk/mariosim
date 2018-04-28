#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ol.h"
#include "sm64/input.h"
#include "sm64/mario.h"
#include "sm64/surface.h"
#include "sm64/types.h"


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


void loadState(OlBlock *in) {
  char *validFields[] = {
    "mario",
    "floor",
    "camera",
    NULL
  };
  ol_checkNoInvalidFields(in, validFields);

  loadMarioState(ol_checkField(in, "mario", ol_block)->block, &marioState);
  loadFloorState(ol_checkField(in, "floor", ol_block)->block, &marioFloor);
  loadCameraState(ol_checkField(in, "camera", ol_block)->block, &cameraAngle);
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

  char *funcNames[] = {
    "solve",
    NULL
  };

  OlBlock *file = ol_parseFile(argv[1], funcNames);
  loadState(file);
  ol_free(file);

  printf("\x1b[1mStarting state:\x1b[0m\n");
  printState();
  printf("\n");

  performStep(0, 127);
  memcpy(&marioState, &endMarioState, sizeof(struct MarioState));

  printf("\x1b[1mEnding state:\x1b[0m\n");
  printState();

  return 0;
}
