#ifndef SCENE_COMPONENTS_H
#define SCENE_COMPONENTS_H

#include "bgfx_components.h"
#include "cglm_components.h"

#include "base.h"

typedef struct Camera {
    vec3   position;
    versor rotation;
    versor inverse_rotation;
    vec3   up;
    float  fov;
    float  near;
    float  far;
    bool   ortho;
    mat4   view;
    mat4   proj;
} Camera;

typedef struct PointLight {
    vec3 position;
    vec3 flux;
} PointLight;

static float calculate_point_light_radius(PointLight *point_light) {
    // radius = where attenuation would lead to an intensity of 1W/m^2
    const float INTENSITY_CUTOFF    = 1.0f;
    const float ATTENTUATION_CUTOFF = 0.05f;
    vec3        intensity;
    glm_vec3_divs(point_light->flux, 4.0f * GLM_PI, intensity);

    float maxIntensity = glm_vec3_max(intensity);
    float attenuation =
        glm_max(INTENSITY_CUTOFF, ATTENTUATION_CUTOFF * maxIntensity) / maxIntensity;
    return 1.0f / sqrtf(attenuation);
}

typedef struct AmbientLight {
    vec3 irradiance;
} AmbientLight;

typedef struct Material {
    bool                  blend;
    bool                  double_sided;
    bgfx_texture_handle_t base_color_texture;
    vec4                  base_color_factor;

    bgfx_texture_handle_t metallic_roughness_texture; // blue = metallic, green = roughness
    float                 metallic_factor;
    float                 roughness_factor;

    bgfx_texture_handle_t normal_texture;
    float                 normal_scale;

    bgfx_texture_handle_t occlusion_texture;
    float                 occlusion_strength;

    bgfx_texture_handle_t emissive_texture;
    vec3                  emissive_factor;

} Material;

typedef struct Sphere {
    vec3  center;
    float radius;
} Sphere;

typedef struct AABB {
    vec3 min;
    vec3 max;
} AABB;

typedef struct OBB {
    mat4 matrix;
} OBB;

typedef struct Primitive {
    uint32_t start_index;
    uint32_t num_indices;
    uint32_t start_vertex;
    uint32_t num_vertices;

    Sphere sphere;
    AABB   aabb;
    OBB    obb;
} Primitive;

typedef struct Group {
    bgfx_vertex_buffer_handle_t vertex_buffer;
    bgfx_index_buffer_handle_t  index_buffer;
    uint16_t                    num_vertices;
    uint8_t                    *vertices;
    uint32_t                    num_indices;
    uint16_t                   *indices;
    Sphere                      sphere;
    AABB                        aabb;
    OBB                         obb;
    ecs_vector_t               *primitives;
} Group;

typedef struct Mesh {
    ecs_vector_t *groups;
} Mesh;

EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(PointLight);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(AmbientLight);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(Material);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(Mesh);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(Camera);

EQUILIBRIUM_API
void SceneComponentsImport(world_t *world);

#endif