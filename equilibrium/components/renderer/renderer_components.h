#ifndef RENDERER_COMPONENTS_H
#define RENDERER_COMPONENTS_H

#include <stdint.h>
#include "bgfx_components.h"
#include "base.h"

static const uint8_t PBR_ALBEDO_LUT = 0;

static const uint8_t PBR_BASECOLOR      = 1;
static const uint8_t PBR_METALROUGHNESS = 2;
static const uint8_t PBR_NORMAL         = 3;
static const uint8_t PBR_OCCLUSION      = 4;
static const uint8_t PBR_EMISSIVE       = 5;

static const uint8_t LIGHTS_POINTLIGHTS = 6;

static const uint8_t CLUSTERS_CLUSTERS     = 7;
static const uint8_t CLUSTERS_LIGHTINDICES = 8;
static const uint8_t CLUSTERS_LIGHTGRID    = 9;
static const uint8_t CLUSTERS_ATOMICINDEX  = 10;

static const uint8_t DEFERRED_DIFFUSE_A          = 7;
static const uint8_t DEFERRED_NORMAL             = 8;
static const uint8_t DEFERRED_F0_METALLIC        = 9;
static const uint8_t DEFERRED_EMISSIVE_OCCLUSION = 10;
static const uint8_t DEFERRED_DEPTH              = 11;

#define ALBEDO_LUT_SIZE    32;
#define ALBEDO_LUT_THREADS 32;

typedef struct TextureBuffer {
    bgfx_texture_handle_t handle;
    const char           *name;
} TextureBuffer;

typedef enum GBufferAttachment {
    // no world position
    // gl_Fragcoord is enough to unproject

    // RGB = diffuse
    // A = a (remapped roughness)
    Diffuse_A,

    // RG = encoded normal
    Normal,

    // RGB = F0 (Fresnel at normal incidence)
    // A = metallic
    // TODO? don't use F0, calculate from diffuse and metallic in shader
    //       where do we store metallic?
    F0_Metallic,

    // RGB = emissive radiance
    // A = occlusion multiplier
    EmissiveOcclusion,

    G_Depth,

    GBufferAttachmentCount
} GBufferAttachment;

static bgfx_texture_format_t gBufferAttachmentFormats[GBufferAttachmentCount - 1] = {
    BGFX_TEXTURE_FORMAT_BGRA8, BGFX_TEXTURE_FORMAT_RG16F, BGFX_TEXTURE_FORMAT_BGRA8,
    BGFX_TEXTURE_FORMAT_BGRA8
    // depth format is determined dynamically
};

static uint64_t gBufferSamplerFlags = BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT |
                                      BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP |
                                      BGFX_SAMPLER_V_CLAMP;

typedef struct LightShader {
    bgfx_uniform_handle_t light_count_vec_uniform;
    bgfx_uniform_handle_t ambient_light_irradiance_uniform;
} LightShader;

typedef struct ForwardRenderer {
    bgfx_program_handle_t program;
} ForwardRenderer;

typedef struct DeferredRenderer {
    bgfx_vertex_buffer_handle_t point_light_vertex_buffer;
    bgfx_index_buffer_handle_t  point_light_index_buffer;

    TextureBuffer
                g_buffer_textures[GBufferAttachmentCount + 1]; // includes depth , + null-terminated
    uint8_t     g_buffer_texture_units[GBufferAttachmentCount];
    const char *g_buffer_sampler_names[GBufferAttachmentCount];
    bgfx_uniform_handle_t      g_buffer_samplers[GBufferAttachmentCount];
    bgfx_frame_buffer_handle_t g_buffer;

    bgfx_texture_handle_t      light_depth_texture;
    bgfx_frame_buffer_handle_t accum_frame_buffer;

    bgfx_uniform_handle_t light_index_vec_uniform;

    bgfx_program_handle_t geometry_program;
    bgfx_program_handle_t fullscreen_program;
    bgfx_program_handle_t point_light_program;
    bgfx_program_handle_t transparency_program;
} DeferredRenderer;

typedef struct PBRShader {
    bgfx_uniform_handle_t base_color_factor_uniform;
    bgfx_uniform_handle_t metallic_roughness_normal_occlusion_factor_uniform;
    bgfx_uniform_handle_t emissive_factor_uniform;
    bgfx_uniform_handle_t has_textures_uniform;
    bgfx_uniform_handle_t multiple_scattering_uniform;
    bgfx_uniform_handle_t albedo_lut_sampler;
    bgfx_uniform_handle_t base_color_sampler;
    bgfx_uniform_handle_t metallic_roughness_sampler;
    bgfx_uniform_handle_t normal_sampler;
    bgfx_uniform_handle_t occlusion_sampler;
    bgfx_uniform_handle_t emissive_sampler;

    bgfx_texture_handle_t albedo_lut_texture;
    bgfx_texture_handle_t default_texture;

    bgfx_program_handle_t albedo_lut_program;
} PBRShader;

typedef struct FrameData {
    TextureBuffer              *texture_buffers;
    bgfx_frame_buffer_handle_t  frame_buffer;
    bgfx_vertex_buffer_handle_t blit_triangle_buffer;
    bgfx_program_handle_t       blit_program;
    bgfx_uniform_handle_t       blit_sampler;
    bgfx_uniform_handle_t       cam_pos_uniform;
    bgfx_uniform_handle_t       normal_matrix_uniform;
    bgfx_uniform_handle_t       exposure_vec_uniform;
    bgfx_uniform_handle_t       tonemapping_mode_vec_uniform;
} FrameData;

EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(LightShader);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(ForwardRenderer);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(DeferredRenderer);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(PBRShader);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(FrameData);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(HotReloadableShader);

EQUILIBRIUM_API
void RendererComponentsImport(world_t *world);

#endif