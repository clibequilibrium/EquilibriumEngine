#ifndef WORLD_H
#define WORLD_H

#include "equilibrium_defines.h"

typedef struct ecs_world_t world_t;

EQUILIBRIUM_API
world_t *world_create();

EQUILIBRIUM_API
void world_destroy(world_t *world);

#endif