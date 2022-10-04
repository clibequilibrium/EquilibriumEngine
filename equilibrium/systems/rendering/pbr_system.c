
#include "pbr_system.h"
#include "bgfx_system.h"
#include "components/renderer/renderer_components.h"
#include "components/gui.h"
#include "utils/bgfx_utils.h"

const int16_t albedo_lut_size    = ALBEDO_LUT_SIZE;
const int16_t albedo_lut_threads = ALBEDO_LUT_THREADS;

const bool  multipleScatteringEnabled = true;
const float whiteFurnaceEnabled       = false;

const float WHITE_FURNACE_RADIANCE = 1.0f;

static bool set_texture_or_default(PBRShader *pbr_shader, uint8_t stage,
                                   bgfx_uniform_handle_t uniform, bgfx_texture_handle_t texture) {
    bool valid = BGFX_HANDLE_IS_VALID(texture);

    if (!valid) {
        bgfx_set_texture(stage, uniform, pbr_shader->default_texture, UINT32_MAX);
    } else {
        bgfx_set_texture(stage, uniform, texture, UINT32_MAX);
    }

    return valid;
}

uint64_t bind_material(PBRShader *pbr_shader, Material *material) {
    float factor_values[4] = {material->metallic_factor, material->roughness_factor,
                              material->normal_scale, material->occlusion_strength};

    bgfx_set_uniform(pbr_shader->base_color_factor_uniform, &material->base_color_factor[0],
                     UINT16_MAX);
    bgfx_set_uniform(pbr_shader->metallic_roughness_normal_occlusion_factor_uniform,
                     &factor_values[0], UINT16_MAX);
    vec4 emissive_factor = {material->emissive_factor[0], material->emissive_factor[1],
                            material->emissive_factor[2], 0.0f};
    bgfx_set_uniform(pbr_shader->emissive_factor_uniform, &emissive_factor[0], UINT16_MAX);

    float has_textures_values[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    const uint32_t has_texture_mask =
        0 |
        ((set_texture_or_default(pbr_shader, PBR_BASECOLOR, pbr_shader->base_color_sampler,
                                 material->base_color_texture)
              ? 1
              : 0)
         << 0) |
        ((set_texture_or_default(pbr_shader, PBR_METALROUGHNESS,
                                 pbr_shader->metallic_roughness_sampler,
                                 material->metallic_roughness_texture)
              ? 1
              : 0)
         << 1) |
        ((set_texture_or_default(pbr_shader, PBR_NORMAL, pbr_shader->normal_sampler,
                                 material->normal_texture)
              ? 1
              : 0)
         << 2) |
        ((set_texture_or_default(pbr_shader, PBR_OCCLUSION, pbr_shader->occlusion_sampler,
                                 material->occlusion_texture)
              ? 1
              : 0)
         << 3) |
        ((set_texture_or_default(pbr_shader, PBR_EMISSIVE, pbr_shader->emissive_sampler,
                                 material->emissive_texture)
              ? 1
              : 0)
         << 4);

    has_textures_values[0] = (float)(has_texture_mask);

    bgfx_set_uniform(pbr_shader->has_textures_uniform, has_textures_values, UINT16_MAX);

    float multiple_scattering_values[4] = {multipleScatteringEnabled ? 1.0f : 0.0f,
                                           whiteFurnaceEnabled ? WHITE_FURNACE_RADIANCE : 0.0f,
                                           0.0f, 0.0f};

    bgfx_set_uniform(pbr_shader->multiple_scattering_uniform, &multiple_scattering_values[0],
                     UINT16_MAX);

    uint64_t state = 0;
    if (material->blend)
        state |= BGFX_STATE_BLEND_ALPHA;
    if (!material->double_sided)
        state |= BGFX_STATE_CULL_CW;

    return state;
}

static void bind_albedo_lut(PBRShader *pbr_shader, bool compute) {
    if (compute)
        bgfx_set_image(PBR_ALBEDO_LUT, pbr_shader->albedo_lut_texture, 0, BGFX_ACCESS_WRITE,
                       BGFX_TEXTURE_FORMAT_COUNT);
    else
        bgfx_set_texture(PBR_ALBEDO_LUT, pbr_shader->albedo_lut_sampler,
                         pbr_shader->albedo_lut_texture, UINT32_MAX);
}

static void generate_albedo_lut(PBRShader *pbr_shader) {
    bind_albedo_lut(pbr_shader, true);
    bgfx_dispatch(0, pbr_shader->albedo_lut_program, albedo_lut_size / albedo_lut_threads,
                  albedo_lut_size / albedo_lut_threads, 1, UINT8_MAX);
}

static void InitializePBRShader(ecs_iter_t *it) {
    entity_t   entity     = (entity_t){it->entities[0], it->world};
    PBRShader *pbr_shader = entity_get_or_add_component(entity, PBRShader);

    pbr_shader->base_color_factor_uniform =
        create_uniform(it->world, "u_baseColorFactor", BGFX_UNIFORM_TYPE_VEC4);
    pbr_shader->metallic_roughness_normal_occlusion_factor_uniform = create_uniform(
        it->world, "u_metallicRoughnessNormalOcclusionFactor", BGFX_UNIFORM_TYPE_VEC4);
    pbr_shader->emissive_factor_uniform =
        create_uniform(it->world, "u_emissiveFactorVec", BGFX_UNIFORM_TYPE_VEC4);
    pbr_shader->has_textures_uniform =
        create_uniform(it->world, "u_hasTextures", BGFX_UNIFORM_TYPE_VEC4);
    pbr_shader->multiple_scattering_uniform =
        create_uniform(it->world, "u_multipleScatteringVec", BGFX_UNIFORM_TYPE_VEC4);
    pbr_shader->albedo_lut_sampler =
        create_uniform(it->world, "s_texAlbedoLUT", BGFX_UNIFORM_TYPE_SAMPLER);
    pbr_shader->base_color_sampler =
        create_uniform(it->world, "s_texBaseColor", BGFX_UNIFORM_TYPE_SAMPLER);
    pbr_shader->metallic_roughness_sampler =
        create_uniform(it->world, "s_texMetallicRoughness", BGFX_UNIFORM_TYPE_SAMPLER);
    pbr_shader->normal_sampler =
        create_uniform(it->world, "s_texNormal", BGFX_UNIFORM_TYPE_SAMPLER);
    pbr_shader->occlusion_sampler =
        create_uniform(it->world, "s_texOcclusion", BGFX_UNIFORM_TYPE_SAMPLER);
    pbr_shader->emissive_sampler =
        create_uniform(it->world, "s_texEmissive", BGFX_UNIFORM_TYPE_SAMPLER);

    pbr_shader->default_texture =
        create_texture_2d(it->world, 1, 1, false, 1, BGFX_TEXTURE_FORMAT_RGBA8, 0, NULL);
    pbr_shader->albedo_lut_texture = create_texture_2d(
        it->world, albedo_lut_size, albedo_lut_size, false, 1, BGFX_TEXTURE_FORMAT_RGBA32F,
        BGFX_SAMPLER_UVW_CLAMP | BGFX_TEXTURE_COMPUTE_WRITE, NULL);

    pbr_shader->albedo_lut_program =
        create_compute_program(it->world, "cs_multiple_scattering_lut.bin");

    generate_albedo_lut(pbr_shader);
    // finish any queued precomputations before rendering the scene
    bgfx_frame(false);

    ecs_trace("PBR System initialized.");
}

static void UpdatePBRShader(ecs_iter_t *it) {
    PBRShader *pbr_shader = ecs_field(it, PBRShader, 1);

    for (int i = 0; i < it->count; i++) {
        bind_albedo_lut(&pbr_shader[i], false);
    }
}

void PBRSystemImport(world_t *world) {
    ECS_TAG(world, OnRender)
    ECS_MODULE(world, PBRSystem);

    ECS_IMPORT(world, RendererComponents);

    ECS_OBSERVER(world, InitializePBRShader, EcsOnSet, renderer.components.PBRShader);
    ECS_SYSTEM(world, UpdatePBRShader, OnRender, renderer.components.PBRShader);
}