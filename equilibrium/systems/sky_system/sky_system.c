#define SKY_SYSTEM_IMPL
#include "components/renderer/renderer_components.h"
#include "components/cglm_components.h"
#include "components/input.h"
#include "utils/bgfx_utils.h"
#include "sky_system.h"
#include "bgfx_system.h"

ECS_COMPONENT_DECLARE(SkyData);

static void UpdateSun(Sun *sun, SkyData *sky_data, int i, float hour) {
    hour -= 12.0f;

    // Calculate sun orbit
    const float day    = 30.0f * (float)(sun[i].month) + 15.0f;
    float       lambda = 280.46f + 0.9856474f * day;
    lambda             = glm_rad(lambda);
    sun[i].delta       = asinf(sinf(sun[i].ecliptic_obliquity) * sinf(lambda));

    // Update sun position
    const float latitude = glm_rad(sun[i].latitude);
    const float hh       = hour * GLM_PI / 12.0f;

    const float azimuth =
        atan2f(sinf(hh), cosf(hh) * sinf(latitude) - tanf(sun[i].delta) * cosf(latitude));
    const float altitude =
        asinf(sinf(latitude) * sinf(sun[i].delta) + cosf(latitude) * cosf(sun[i].delta) * cosf(hh));

    versor rot0 = GLM_QUAT_IDENTITY_INIT;
    glm_quatv(rot0, azimuth, sun[i].up_dir);

    vec3 dir;
    glm_quat_rotatev(rot0, sun[i].north_dir, dir);
    vec3 uxd;
    glm_cross(sun[i].up_dir, dir, uxd);

    versor rot1 = GLM_QUAT_IDENTITY_INIT;
    glm_quatv(rot1, -altitude, uxd);

    glm_quat_rotatev(rot1, dir, sun[i].sun_dir);

    if (sun[i].latitude == 50.0f && (int)(sky_data[i].time) == 13 && sun[i].month == JUNE &&
        (sun[i].sun_dir[1] < 0.0f || sun[i].sun_dir[2] > 0.0f)) {
        ecs_err("Sun dir is incorrect.");
    }
}

static void InitializeSkySystem(ecs_iter_t *it) {
    entity_t entity = entity_create_empty(it->world, "SkySystemData");

    SkyData *sky_data = entity_get_or_add_component(entity, SkyData);
    Sun     *sun      = entity_get_or_add_component(entity, Sun);

    sun->latitude           = 50.0f;
    sun->month              = JUNE;
    sun->ecliptic_obliquity = glm_rad(23.4f);
    sun->delta              = 0.0f;

    sun->north_dir[0] = 1.0f;
    sun->north_dir[1] = 0.0f;
    sun->north_dir[2] = 0.0f;

    sun->sun_dir[0] = 0.0f;
    sun->sun_dir[1] = -1.0f;
    sun->sun_dir[2] = 0.0f;

    sun->up_dir[0] = 0.0f;
    sun->up_dir[1] = 1.0f;
    sun->up_dir[2] = 0.0f;

    bgfx_vertex_layout_begin(&sky_data->screen_pos_vertex, bgfx_get_renderer_type());
    bgfx_vertex_layout_add(&sky_data->screen_pos_vertex, BGFX_ATTRIB_POSITION, 2,
                           BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_end(&sky_data->screen_pos_vertex);

    const int vertical_count   = 32;
    const int horizontal_count = 32;
    const int vertices_size    = vertical_count * horizontal_count * sizeof(ScreenPosVertex);
    const int indices_size = (vertical_count - 1) * (horizontal_count - 1) * 6 * sizeof(uint16_t);

    ScreenPosVertex *vertices = ecs_os_malloc(vertices_size);
    uint16_t        *indices  = ecs_os_malloc(indices_size);

    for (int i = 0; i < vertical_count; i++) {
        for (int j = 0; j < horizontal_count; j++) {
            ScreenPosVertex *v = &(vertices[i * vertical_count + j]);
            v->x               = (float)(j) / (horizontal_count - 1) * 2.0f - 1.0f;
            v->y               = (float)(i) / (vertical_count - 1) * 2.0f - 1.0f;
        }
    }

    int k = 0;
    for (int i = 0; i < vertical_count - 1; i++) {
        for (int j = 0; j < horizontal_count - 1; j++) {
            indices[k++] = (uint16_t)(j + 0 + horizontal_count * (i + 0));
            indices[k++] = (uint16_t)(j + 1 + horizontal_count * (i + 0));
            indices[k++] = (uint16_t)(j + 0 + horizontal_count * (i + 1));

            indices[k++] = (uint16_t)(j + 1 + horizontal_count * (i + 0));
            indices[k++] = (uint16_t)(j + 1 + horizontal_count * (i + 1));
            indices[k++] = (uint16_t)(j + 0 + horizontal_count * (i + 1));
        }
    }

    UpdateSun(sun, sky_data, 0, 0);

    sky_data->vbh = create_vertex_buffer(
        it->world, bgfx_copy(vertices, sizeof(ScreenPosVertex) * vertical_count * horizontal_count),
        &sky_data->screen_pos_vertex, BGFX_BUFFER_NONE);

    sky_data->ibh =
        create_index_buffer(it->world, bgfx_copy(indices, sizeof(uint16_t) * k), BGFX_BUFFER_NONE);

    sky_data->time       = 17.0f;
    sky_data->time_scale = 0;

    sky_data->u_sunLuminance = create_uniform(it->world, "u_sunLuminance", BGFX_UNIFORM_TYPE_VEC4);
    sky_data->u_skyLuminanceXYZ =
        create_uniform(it->world, "u_skyLuminanceXYZ", BGFX_UNIFORM_TYPE_VEC4);
    sky_data->u_skyLuminance = create_uniform(it->world, "u_skyLuminance", BGFX_UNIFORM_TYPE_VEC4);
    sky_data->u_sunDirection = create_uniform(it->world, "u_sunDirection", BGFX_UNIFORM_TYPE_VEC4);
    sky_data->u_parameters   = create_uniform(it->world, "u_parameters", BGFX_UNIFORM_TYPE_VEC4);
    sky_data->u_perezCoeff =
        create_uniform_w_num(it->world, "u_perezCoeff", BGFX_UNIFORM_TYPE_VEC4, 5);

    sky_data->turbidity = 2.15f;
    sky_data->sky_program =
        create_program(entity, sky_program, SkyData, "vs_sky.bin", "fs_sky.bin");

    ecs_os_free(vertices);
    ecs_os_free(indices);
}

static void DrawSky(ecs_iter_t *it) {
    SkyData *sky_data = ecs_field(it, SkyData, 1);
    Sun     *sun      = ecs_field(it, Sun, 2);
    Input   *input    = ecs_field(it, Input, 3);

    for (int i = 0; i < it->count; i++) {
        sky_data[i].time += sky_data[i].time_scale * it->delta_time;
        sky_data[i].time = mod(sky_data[i].time, 24.0f);

        UpdateSun(sun, sky_data, i, sky_data[i].time);

        Color sunLuminanceXYZ = getSunLuminanceXYZTableValue(sky_data[i].time);
        Color sunLuminanceRGB = xyzToRgb(&sunLuminanceXYZ);

        Color skyLuminanceXYZ = getSkyLuminanceXYZTablefloat(sky_data[i].time);
        Color skyLuminanceRGB = xyzToRgb(&skyLuminanceXYZ);

        bgfx_set_uniform(sky_data[i].u_sunLuminance, &sunLuminanceRGB.x, UINT16_MAX);
        bgfx_set_uniform(sky_data[i].u_skyLuminanceXYZ, &skyLuminanceXYZ.x, UINT16_MAX);
        bgfx_set_uniform(sky_data[i].u_skyLuminance, &skyLuminanceRGB.x, UINT16_MAX);
        bgfx_set_uniform(sky_data[i].u_sunDirection, &sun[i].sun_dir[0], UINT16_MAX);

        float exposition[4] = {0.02f, 3.0f, 0.1f, sky_data[i].time};
        bgfx_set_uniform(sky_data[i].u_parameters, exposition, UINT16_MAX);

        float perezCoeff[4 * 5];
        computePerezCoeff(sky_data[i].turbidity, perezCoeff);
        bgfx_set_uniform(sky_data[i].u_perezCoeff, perezCoeff, 5);

        // Draw
        bgfx_set_state(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_EQUAL, 0);
        bgfx_set_index_buffer(sky_data[i].ibh, 0, UINT32_MAX);
        bgfx_set_vertex_buffer(0, sky_data[i].vbh, 0, UINT32_MAX);

        bgfx_view_id_t viewId = ecs_count(it->world, DeferredRenderer) > 0 ? 3 : 0;
        bgfx_submit(viewId, sky_data[i].sky_program, 0, BGFX_DISCARD_ALL);
    }
}

void SkySystemImport(world_t *world) {
    ECS_TAG(world, OnRender);

    ECS_MODULE(world, SkySystem);
    ECS_IMPORT(world, BgfxSystem);
    ECS_IMPORT(world, RendererComponents);
    ECS_IMPORT(world, InputComponents);

    ECS_COMPONENT_DEFINE(world, SkyData);

    ECS_META_COMPONENT(world, Month);
    ECS_META_COMPONENT(world, Sun);

    ECS_OBSERVER(world, InitializeSkySystem, EcsOnSet, [in] bgfx.components.Bgfx);
    ECS_SYSTEM(world, DrawSky, OnRender, sky.system.SkyData, sky.system.Sun,
               input.components.Input($));
}