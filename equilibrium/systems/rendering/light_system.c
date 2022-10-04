
#include "light_system.h"
#include "bgfx_system.h"
#include "components/renderer/renderer_components.h"
#include "components/gui.h"
#include "utils/bgfx_utils.h"

static bgfx_dynamic_vertex_buffer_handle_t buffer;
static bgfx_vertex_layout_t                layout;

typedef struct PointLightVertex {
    vec3  position;
    float padding;

    // radiant intensity in W/sr
    // can be calculated from radiant flux
    vec3  intensity;
    float radius;
} PointLightVertex;

static void InitializeLightShader(ecs_iter_t *it) {

    entity_t     entity       = (entity_t){it->entities[0], it->world};
    LightShader *light_shader = entity_get_or_add_component(entity, LightShader);

    light_shader->light_count_vec_uniform =
        create_uniform(it->world, "u_lightCountVec", BGFX_UNIFORM_TYPE_VEC4);
    light_shader->ambient_light_irradiance_uniform =
        create_uniform(it->world, "u_ambientLightIrradiance", BGFX_UNIFORM_TYPE_VEC4);

    bgfx_vertex_layout_begin(&layout, bgfx_get_renderer_type());
    bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_TEXCOORD0, 4, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_add(&layout, BGFX_ATTRIB_TEXCOORD1, 4, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_end(&layout);

    buffer = create_dynamic_vertex_buffer(it->world, 1, &layout,
                                          BGFX_BUFFER_COMPUTE_READ | BGFX_BUFFER_ALLOW_RESIZE);

    // finish any queued precomputations before rendering the scene
    bgfx_frame(false);

    ecs_trace("Light System initialized.");
}

static void BindPointLights(ecs_iter_t *it) {
    LightShader *light_shader = ecs_field(it, LightShader, 1);

    for (int i = 0; i < it->count; i++) {

        int lights_count = 0;

        ecs_iter_t point_lights_iterator = ecs_query_iter(it->world, it->ctx);
        while (ecs_query_next(&point_lights_iterator)) {
            lights_count += point_lights_iterator.count;
        }

        // a 32-bit IEEE 754 float can represent all integers up to 2^24 (~16.7
        // million) correctly should be enough for this use case (comparison in
        // for loop)
        float lightCountVec[4] = {(float)lights_count};

        bgfx_set_uniform(light_shader[i].light_count_vec_uniform, &lightCountVec[0], UINT16_MAX);

        // TODO: ambient light entity
        vec4 ambient_light_irradiance = {0.25f, 0.25f, 0.25f, 1.0f};
        bgfx_set_uniform(light_shader[i].ambient_light_irradiance_uniform,
                         &ambient_light_irradiance[0], UINT16_MAX);

        bgfx_set_compute_dynamic_vertex_buffer(LIGHTS_POINTLIGHTS, buffer, BGFX_ACCESS_READ);
    }
}

static void UpdatePointLights(ecs_iter_t *it) {
    PointLight *point_light = ecs_field(it, PointLight, 1);

    size_t               stride = layout.stride;
    const bgfx_memory_t *mem    = bgfx_alloc((uint32_t)(stride * it->count)); // todo improve max

    for (int i = 0; i < it->count; i++) {
        PointLightVertex *light = (PointLightVertex *)(mem->data + (i * stride));
        glm_vec3_copy(point_light[i].position, light->position);

        // intensity = flux per unit solid angle (steradian)
        // there are 4*pi steradians in a sphere
        glm_vec3_divs(point_light[i].flux, 4.0f * GLM_PI, light->intensity);
        light->radius = calculate_point_light_radius(&point_light[i]);
    }

    bgfx_update_dynamic_vertex_buffer(buffer, 0, mem);
}

void LightSystemImport(world_t *world) {
    ECS_TAG(world, OnRender)
    ECS_MODULE(world, LightSystem);

    ECS_IMPORT(world, SceneComponents);
    ECS_IMPORT(world, RendererComponents);

    ECS_OBSERVER(world, InitializeLightShader, EcsOnSet, renderer.components.LightShader);
    ECS_SYSTEM(world, UpdatePointLights, EcsOnUpdate, scene.components.PointLight);

    ECS_SYSTEM(world, BindPointLights, OnRender, renderer.components.LightShader);
    ecs_system(world, {.entity = BindPointLights, .ctx = ecs_query_new(world, "PointLight")});
}