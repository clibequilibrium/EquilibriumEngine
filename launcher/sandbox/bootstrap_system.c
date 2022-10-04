#include "bootstrap_system.h"
#include "bgfx/c99/bgfx.h"
#include "cglm/types.h"
#include "cglm/vec4.h"
#include "entity.h"
#include "scene/scene_components.h"
#include "stdbool.h"
#include "utils/bgfx_utils.h"
#include <assert.h>

static void Bootstrap(ecs_iter_t *it) {

    assimp_scene_load("models/Sponza/glTF/Sponza.gltf", it->world);
    // cgltf_model_load("models/Sponza/glTF/Sponza.gltf", it->world);

    entity_create(it->world, "Point Light", PointLight, {{-5.0f, 1.3f, 0.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{-5.0f, 1.3f, 0.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{0.0f, 1.3f, 0.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{5.0f, 1.3f, 0.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{10.0f, 1.3f, 0.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{-10.0f, 1.3f, 0.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{-5.0f, 5, -3.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{0.0f, 5, -3.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{5.0f, 5, -3.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{10.0f, 5, -3.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{-10.0f, 5, -3.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{-5.0f, 5, 3.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{0.0f, 5, 3.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{5.0f, 5, 3.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{10.0f, 5, 3.0f}, {100, 100, 100}});
    entity_create(it->world, "Point Light", PointLight, {{-10.0f, 5, 3.0f}, {100, 100, 100}});
}

static void MoveLights(ecs_iter_t *it) {
    PointLight *point_lights = ecs_field(it, PointLight, 1);

    const float angularVelocity = glm_rad(25.0f);
    const float angle           = angularVelocity * it->delta_time;

    for (size_t i = 0; i < it->count; i++) {

        mat4 matrix = GLM_MAT4_IDENTITY_INIT;
        mat3 temp;
        glm_rotate(matrix, angle, (vec3){0, 1, 0});
        glm_mat4_pick3(matrix, temp);

        glm_mat3_mulv(temp, point_lights[i].position, point_lights[i].position);
    }
}

void BootstrapSystemImport(world_t *world) {
    ECS_MODULE(world, BootstrapSystem);

    ECS_OBSERVER(world, Bootstrap, EcsOnSet, [in] bgfx.components.Bgfx);
    ECS_SYSTEM(world, MoveLights, EcsOnUpdate, scene.components.PointLight);
}
