#ifndef BGFX_COMPONENT_H
#define BGFX_COMPONENT_H

#include <bgfx/c99/bgfx.h>
#include "base.h"

typedef struct Bgfx {
    bgfx_init_t data;
    uint32_t    reset;
} Bgfx;

EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(Bgfx);

EQUILIBRIUM_API
void BgfxComponentsImport(world_t *world);

#endif