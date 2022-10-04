#include "hot_reloadable_module.h"

ECS_COMPONENT_DECLARE(HotReloadableModule);

void HotReloadableComponentsImport(world_t *world) {
    ECS_MODULE(world, HotReloadableComponents);
    ECS_COMPONENT_DEFINE(world, HotReloadableModule);
}