#include "sm64/input.h"

#include "sm64/types.h"

#include <math.h>


void adjust_analog_stick(struct Controller *controller)
{
    UNUSED u8 pad[8];

    // reset the controller's x and y floats.
    controller->stickX = 0;
    controller->stickY = 0;

    // modulate the rawStickX and rawStickY to be the new float values by adding/subtracting 6.
    if(controller->rawStickX <= -8)
        controller->stickX = controller->rawStickX + 6;

    if(controller->rawStickX >=  8)
        controller->stickX = controller->rawStickX - 6;

    if(controller->rawStickY <= -8)
        controller->stickY = controller->rawStickY + 6;

    if(controller->rawStickY >=  8)
        controller->stickY = controller->rawStickY - 6;

    // calculate float magnitude from the center by vector length.
    controller->stickMag = sqrtf(controller->stickX * controller->stickX 
                               + controller->stickY * controller->stickY);

    // magnitude cannot exceed 64.0f: if it does, modify the values appropriately to
    // flatten the values down to the allowed maximum value.
    if(controller->stickMag > 64)
    {
        controller->stickX  *= 64 / controller->stickMag;
        controller->stickY  *= 64 / controller->stickMag;
        controller->stickMag = 64;
    }
}
