#include "scene_components.h"

ECS_COMPONENT_DECLARE(PointLight);
ECS_COMPONENT_DECLARE(AmbientLight);
ECS_COMPONENT_DECLARE(Material);
ECS_COMPONENT_DECLARE(Mesh);
ECS_COMPONENT_DECLARE(Camera);

void SceneComponentsImport(world_t *world) {
    ECS_MODULE(world, SceneComponents);

    ECS_COMPONENT_DEFINE(world, PointLight);
    ECS_COMPONENT_DEFINE(world, AmbientLight);
    ECS_COMPONENT_DEFINE(world, Material);
    ECS_COMPONENT_DEFINE(world, Mesh);

    ECS_IMPORT(world, CglmComponents);
    ECS_COMPONENT_DEFINE(world, Camera)
}