#ifndef SKY_SYSTEM_H
#define SKY_SYSTEM_H

#include "world.h"
#include "sky_color_map.h"

#undef ECS_META_IMPL
#ifndef SKY_SYSTEM_IMPL
#define ECS_META_IMPL EXTERN // Ensure meta symbols are only defined once
#endif

typedef struct ScreenPosVertex {
    float x;
    float y;
} ScreenPosVertex;

static float M_XYZ2RGB[] = {
    3.240479f,  -0.969256f, 0.055648f, -1.53715f, 1.875991f,
    -0.204043f, -0.49853f,  0.041556f, 1.057311f,
};

static Color xyzToRgb(const Color *xyz) {
    Color rgb = {0, 0, 0};
    rgb.x     = M_XYZ2RGB[0] * xyz->x + M_XYZ2RGB[3] * xyz->y + M_XYZ2RGB[6] * xyz->z;
    rgb.y     = M_XYZ2RGB[1] * xyz->x + M_XYZ2RGB[4] * xyz->y + M_XYZ2RGB[7] * xyz->z;
    rgb.z     = M_XYZ2RGB[2] * xyz->x + M_XYZ2RGB[5] * xyz->y + M_XYZ2RGB[8] * xyz->z;
    return rgb;
};

static Color ABCDE[] = {
    {-0.2592f, -0.2608f, -1.4630f}, {0.0008f, 0.0092f, 0.4275f}, {0.2125f, 0.2102f, 5.3251f},
    {-0.8989f, -1.6537f, -2.5771f}, {0.0452f, 0.0529f, 0.3703f},
};

static Color ABCDE_t[] = {
    {-0.0193f, -0.0167f, 0.1787f}, {-0.0665f, -0.0950f, -0.3554f}, {-0.0004f, -0.0079f, -0.0227f},
    {-0.0641f, -0.0441f, 0.1206f}, {-0.0033f, -0.0109f, -0.0670f},
};

static void computePerezCoeff(float _turbidity, float *_outPerezCoeff) {
    vec3 turbidity = {_turbidity, _turbidity, _turbidity};
    for (uint32_t ii = 0; ii < 5; ++ii) {
        vec3 tmp;
        vec3 mulResult;

        vec3 a = {ABCDE_t[ii].x, ABCDE_t[ii].y, ABCDE_t[ii].z};
        vec3 b = {ABCDE[ii].x, ABCDE[ii].y, ABCDE[ii].z};

        glm_vec3_mul(a, turbidity, mulResult);
        glm_vec3_add(mulResult, b, tmp);

        float *out = _outPerezCoeff + 4 * ii;
        ecs_os_memcpy(out, tmp, sizeof(vec3));
        out[3] = 0.0f;
    }
}

typedef struct SkyData {
    bgfx_vertex_layout_t screen_pos_vertex;

    bgfx_vertex_buffer_handle_t vbh;
    bgfx_index_buffer_handle_t  ibh;
    bgfx_program_handle_t       sky_program;

    bgfx_uniform_handle_t u_sunLuminance;
    bgfx_uniform_handle_t u_skyLuminanceXYZ;
    bgfx_uniform_handle_t u_skyLuminance;
    bgfx_uniform_handle_t u_sunDirection;
    bgfx_uniform_handle_t u_parameters;
    bgfx_uniform_handle_t u_perezCoeff;

    float time;
    float time_scale;
    float turbidity;

} SkyData;

ECS_ENUM(Month, {JANUARY, FEBRUARY, MARCH, APRIL, MAY, JUNE, JULY, AUGUST, SEPTEMBER, OCTOBER,
                 NOVEMBER, DECEMBER});

ECS_STRUCT(Sun, {
    vec3  north_dir;
    vec3  sun_dir;
    vec3  up_dir;
    float latitude;
    Month month;

    float ecliptic_obliquity;
    float delta;
});

EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(SkyData);

EQUILIBRIUM_API
void SkySystemImport(world_t *world);

#endif