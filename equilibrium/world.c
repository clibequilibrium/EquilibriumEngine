#include <flecs.h>
#include "world.h"

world_t *world_create() { return ecs_init(); }

void world_destroy(world_t *world) { return ecs_quit(world); }