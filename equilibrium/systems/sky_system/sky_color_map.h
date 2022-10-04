#ifndef SKY_MAP_H
#define SKY_MAP_H

#include "bgfx_components.h"

typedef struct Color {
    float x;
    float y;
    float z;
} Color;

BGFX_C_API Color getSunLuminanceXYZTableValue(float time);
BGFX_C_API Color getSkyLuminanceXYZTablefloat(float time);

#endif