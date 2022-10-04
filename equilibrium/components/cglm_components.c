#include "cglm_components.h"

ECS_COMPONENT_DECLARE(vec3);
ECS_COMPONENT_DECLARE(versor);

void CglmComponentsImport(world_t *world) {
    ECS_MODULE(world, CglmComponents);

    ecs_id(vec3) = ecs_array(
        world, {.entity = ecs_entity(world, {.name = "vec3", .symbol = "vec3", .id = ecs_id(vec3)}),
                .type   = ecs_id(ecs_f32_t),
                .count  = 3});

    ecs_id(versor) = ecs_array(
        world,
        {.entity = ecs_entity(world, {.name = "versor", .symbol = "versor", .id = ecs_id(versor)}),
         .type   = ecs_id(ecs_f32_t),
         .count  = 4});
}