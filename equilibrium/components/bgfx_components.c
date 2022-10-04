#include "bgfx_components.h"

ECS_COMPONENT_DECLARE(Bgfx);

void BgfxComponentsImport(world_t *world) {
    ECS_MODULE(world, BgfxComponents);
    ECS_COMPONENT_DEFINE(world, Bgfx);
}