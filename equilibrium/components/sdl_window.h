#ifndef SDL_WINDOW_H
#define SDL_WINDOW_H

#include "base.h"

typedef struct SdlWindow {
    int32_t dummy;
} SdlWindow;

EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(SdlWindow);

EQUILIBRIUM_API
void SdlComponentsImport(world_t *world);

#endif