#include "transform.h"

ECS_COMPONENT_DECLARE(Position);
ECS_COMPONENT_DECLARE(Scale);
ECS_COMPONENT_DECLARE(Rotation);
ECS_COMPONENT_DECLARE(Quaternion);
ECS_COMPONENT_DECLARE(Transform);
ECS_COMPONENT_DECLARE(Project);

void TransformComponentsImport(world_t *world) {
    ECS_MODULE(world, TransformComponents);
    ECS_IMPORT(world, CglmComponents);

    ECS_COMPONENT_DEFINE(world, Position);
    ECS_COMPONENT_DEFINE(world, Scale);
    ECS_COMPONENT_DEFINE(world, Rotation);
    ECS_COMPONENT_DEFINE(world, Quaternion);
    ECS_COMPONENT_DEFINE(world, Transform);
    ECS_COMPONENT_DEFINE(world, Project);
}