
#include "deferred_renderer_system.h"
#include "base_rendering_system.h"
#include "pbr_system.h"
#include "light_system.h"
#include "bgfx_system.h"
#include "scene/camera_system.h"
#include "components/renderer/renderer_components.h"
#include "components/gui.h"
#include "utils/bgfx_utils.h"

static bgfx_view_id_t vGeometry        = 0;
static bgfx_view_id_t vFullscreenLight = 1;
static bgfx_view_id_t vLight           = 2;
static bgfx_view_id_t vTransparent     = 3;

static void *ctx;

static bgfx_frame_buffer_handle_t create_g_buffer(world_t          *world,
                                                  DeferredRenderer *deferred_render) {
    bgfx_texture_handle_t textures[GBufferAttachmentCount];

    const uint64_t flags = BGFX_TEXTURE_RT | gBufferSamplerFlags;

    for (size_t i = 0; i < G_Depth; i++) {
        assert(bgfx_is_texture_valid(0, false, 1, gBufferAttachmentFormats[i], flags));
        textures[i] = create_texture_2d_scaled(world, BGFX_BACKBUFFER_RATIO_EQUAL, false, 1,
                                               gBufferAttachmentFormats[i], flags);
    }

    bgfx_texture_format_t depthFormat = find_depth_format(flags, false);
    assert(depthFormat != BGFX_TEXTURE_FORMAT_COUNT);
    textures[G_Depth] =
        create_texture_2d_scaled(world, BGFX_BACKBUFFER_RATIO_EQUAL, false, 1, depthFormat, flags);

    bgfx_frame_buffer_handle_t gb =
        create_frame_buffer_from_handles(world, (uint8_t)GBufferAttachmentCount, textures);

    if (!BGFX_HANDLE_IS_VALID(gb))
        ecs_err("Failed to create G-Buffer");
    else
        bgfx_set_frame_buffer_name(gb, "G-Buffer", INT32_MAX);

    return gb;
}

static void bind_g_buffer(DeferredRenderer *deferred_renderer) {
    for (size_t i = 0; i < GBufferAttachmentCount; i++) {
        bgfx_set_texture(deferred_renderer->g_buffer_texture_units[i],
                         deferred_renderer->g_buffer_samplers[i],
                         deferred_renderer->g_buffer_textures[i].handle, UINT32_MAX);
    }
}

static void OnAppWindowResized(ecs_iter_t *it) {

    ecs_iter_t components_iterator = ecs_query_iter(it->world, ctx);

    while (ecs_query_next(&components_iterator)) {
        FrameData        *frame_data        = ecs_field(&components_iterator, FrameData, 1);
        DeferredRenderer *deferred_renderer = ecs_field(&components_iterator, DeferredRenderer, 2);

        for (int i = 0; i < components_iterator.count; i++) {
            if (!BGFX_HANDLE_IS_VALID(deferred_renderer[i].g_buffer)) {
                deferred_renderer[i].g_buffer = create_g_buffer(it->world, deferred_renderer);

                for (size_t texture_id = 0; texture_id < G_Depth; texture_id++) {
                    deferred_renderer[i].g_buffer_textures[texture_id].handle =
                        bgfx_get_texture(deferred_renderer[i].g_buffer, (uint8_t)texture_id);
                }

                // we can't use the G-Buffer's depth texture in the light pass
                // framebuffer binding a texture for reading in the shader and
                // attaching it to a framebuffer at the same time is undefined
                // behaviour in most APIs
                // https://www.khronos.org/opengl/wiki/Memory_Model#Framebuffer_objects
                // we use a different depth texture and just blit it between the
                // geometry and light pass
                const uint64_t        flags       = BGFX_TEXTURE_BLIT_DST | gBufferSamplerFlags;
                bgfx_texture_format_t depthFormat = find_depth_format(flags, false);
                deferred_renderer[i].light_depth_texture = create_texture_2d_scaled(
                    it->world, BGFX_BACKBUFFER_RATIO_EQUAL, false, 1, depthFormat, flags);

                deferred_renderer[i].g_buffer_textures[G_Depth].handle =
                    deferred_renderer[i].light_depth_texture;
            }

            if (!BGFX_HANDLE_IS_VALID(deferred_renderer[i].accum_frame_buffer)) {
                const bgfx_texture_handle_t textures[2] = {
                    bgfx_get_texture(frame_data[i].frame_buffer, 0),
                    bgfx_get_texture(deferred_renderer[i].g_buffer, G_Depth)};

                deferred_renderer[i].accum_frame_buffer =
                    create_frame_buffer_from_handles(it->world, BX_COUNT_OF(textures), textures);
            }
        }
    }
}

static void InitializeDeferredRenderer(ecs_iter_t *it) {

    if (!renderer_supported(true)) {
        ecs_err("Deferred rendering is not supported on this device");
        return;
    }

    entity_t          entity            = (entity_t){it->entities[0], it->world};
    DeferredRenderer *deferred_renderer = entity_get_or_add_component(entity, DeferredRenderer);

    deferred_renderer->g_buffer_textures[0] =
        (TextureBuffer){BGFX_INVALID_HANDLE, "Diffuse + roughness"};
    deferred_renderer->g_buffer_textures[1] = (TextureBuffer){BGFX_INVALID_HANDLE, "Normal"};
    deferred_renderer->g_buffer_textures[2] = (TextureBuffer){BGFX_INVALID_HANDLE, "F0 + metallic"};
    deferred_renderer->g_buffer_textures[3] =
        (TextureBuffer){BGFX_INVALID_HANDLE, "Emissive + occlusion"};
    deferred_renderer->g_buffer_textures[4] = (TextureBuffer){BGFX_INVALID_HANDLE, "G_Depth"};
    deferred_renderer->g_buffer_textures[5] = (TextureBuffer){BGFX_INVALID_HANDLE, NULL};

    deferred_renderer->g_buffer_texture_units[0] = DEFERRED_DIFFUSE_A;
    deferred_renderer->g_buffer_texture_units[1] = DEFERRED_NORMAL;
    deferred_renderer->g_buffer_texture_units[2] = DEFERRED_F0_METALLIC;
    deferred_renderer->g_buffer_texture_units[3] = DEFERRED_EMISSIVE_OCCLUSION;
    deferred_renderer->g_buffer_texture_units[4] = DEFERRED_DEPTH;

    deferred_renderer->g_buffer_sampler_names[0] = "s_texDiffuseA";
    deferred_renderer->g_buffer_sampler_names[1] = "s_textNormal";
    deferred_renderer->g_buffer_sampler_names[2] = "s_textF0Metallic";
    deferred_renderer->g_buffer_sampler_names[3] = "s_texEmissiveOcclusion";
    deferred_renderer->g_buffer_sampler_names[4] = "s_texDepth";

    for (size_t i = 0; i < BX_COUNT_OF(deferred_renderer->g_buffer_samplers); i++) {
        deferred_renderer->g_buffer_samplers[i] = create_uniform(
            it->world, deferred_renderer->g_buffer_sampler_names[i], BGFX_UNIFORM_TYPE_SAMPLER);
    }

    deferred_renderer->light_index_vec_uniform =
        create_uniform(it->world, "u_lightIndexVec", BGFX_UNIFORM_TYPE_VEC4);

    deferred_renderer->g_buffer            = (bgfx_frame_buffer_handle_t)BGFX_INVALID_HANDLE;
    deferred_renderer->light_depth_texture = (bgfx_texture_handle_t)BGFX_INVALID_HANDLE;
    deferred_renderer->accum_frame_buffer  = (bgfx_frame_buffer_handle_t)BGFX_INVALID_HANDLE;

    // axis-aligned bounding box used as light geometry for light culling
    const float LEFT = -1.0f, RIGHT = 1.0f, BOTTOM = -1.0f, TOP = 1.0f, FRONT = -1.0f, BACK = 1.0f;
    const PosVertex vertices[8] = {
        {LEFT, BOTTOM, FRONT}, {RIGHT, BOTTOM, FRONT}, {LEFT, TOP, FRONT}, {RIGHT, TOP, FRONT},
        {LEFT, BOTTOM, BACK},  {RIGHT, BOTTOM, BACK},  {LEFT, TOP, BACK},  {RIGHT, TOP, BACK},
    };

    bgfx_vertex_layout_t pcvDecl;

    bgfx_vertex_layout_begin(&pcvDecl, bgfx_get_renderer_type());
    bgfx_vertex_layout_add(&pcvDecl, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_end(&pcvDecl);

    const uint16_t indices[6 * 6] = {
        // CCW
        0, 1, 3, 3, 2, 0, // front
        5, 4, 6, 6, 7, 5, // back
        4, 0, 2, 2, 6, 4, // left
        1, 5, 7, 7, 3, 1, // right
        2, 3, 7, 7, 6, 2, // top
        4, 5, 1, 1, 0, 4  // bottom
    };

    deferred_renderer->point_light_vertex_buffer = create_vertex_buffer(
        it->world, bgfx_copy(vertices, sizeof(vertices)), &pcvDecl, BGFX_BUFFER_NONE);
    deferred_renderer->point_light_index_buffer =
        create_index_buffer(it->world, bgfx_copy(indices, sizeof(indices)), BGFX_BUFFER_NONE);

    deferred_renderer->geometry_program =
        create_program(entity, geometry_program, DeferredRenderer, "vs_deferred_geometry.bin",
                       "fs_deferred_geometry.bin");

    deferred_renderer->fullscreen_program =
        create_program(entity, fullscreen_program, DeferredRenderer, "vs_deferred_fullscreen.bin",
                       "fs_deferred_fullscreen.bin");

    deferred_renderer->point_light_program =
        create_program(entity, point_light_program, DeferredRenderer, "vs_deferred_light.bin",
                       "fs_deferred_pointlight.bin");

    deferred_renderer->transparency_program = create_program(
        entity, transparency_program, DeferredRenderer, "vs_forward.bin", "fs_forward.bin");

    ecs_set(it->world, it->entities[0], FrameData, {.frame_buffer = BGFX_INVALID_HANDLE});
    ecs_set(it->world, it->entities[0], PBRShader, {.albedo_lut_program = BGFX_INVALID_HANDLE});
    ecs_set(it->world, it->entities[0], LightShader,
            {.light_count_vec_uniform = BGFX_INVALID_HANDLE});

    OnAppWindowResized(it);

    ecs_trace("Deferred rendering System initialized");
}

static void DeferredRendererBeginFrame(ecs_iter_t *it) {

    DeferredRenderer *deferred_renderer = ecs_field(it, DeferredRenderer, 1);
    AppWindow        *app_window        = ecs_field(it, AppWindow, 2);
    FrameData        *frame_data        = ecs_field(it, FrameData, 3);
    Camera           *camera            = ecs_field(it, Camera, 4);

    for (int i = 0; i < it->count; i++) {

        if (!BGFX_HANDLE_IS_VALID(frame_data[i].frame_buffer))
            continue;

        const uint32_t BLACK  = 0x000000FF;
        int            width  = app_window[i].width;
        int            height = app_window[i].height;

        bgfx_set_view_name(vGeometry, "Deferred geometry pass");
        bgfx_set_view_clear(vGeometry, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, BLACK, 1.0f, 0);
        bgfx_set_view_rect(vGeometry, 0, 0, width, height);
        bgfx_set_view_frame_buffer(vGeometry, deferred_renderer->g_buffer);
        bgfx_touch(vGeometry);

        bgfx_set_view_name(vFullscreenLight, "Deferred light pass (sun + ambient + emissive)");
        bgfx_set_view_clear(vFullscreenLight, BGFX_CLEAR_COLOR, 0x303030FF, 1.0f, 0.0);
        bgfx_set_view_rect(vFullscreenLight, 0, 0, width, height);
        bgfx_set_view_frame_buffer(vFullscreenLight, deferred_renderer->accum_frame_buffer);
        bgfx_touch(vFullscreenLight);

        bgfx_set_view_name(vLight, "Deferred light pass (point lights)");
        bgfx_set_view_clear(vLight, BGFX_CLEAR_NONE, 255, 1.0f, 0);
        bgfx_set_view_rect(vLight, 0, 0, width, height);
        bgfx_set_view_frame_buffer(vLight, deferred_renderer->accum_frame_buffer);
        bgfx_touch(vLight);

        bgfx_set_view_name(vTransparent, "Transparent forward pass");
        bgfx_set_view_clear(vTransparent, BGFX_CLEAR_NONE, 255, 1.0f, 0);
        bgfx_set_view_rect(vTransparent, 0, 0, width, height);
        bgfx_set_view_frame_buffer(vTransparent, deferred_renderer->accum_frame_buffer);
        bgfx_touch(vTransparent);

        set_view_projection(vGeometry, &camera[i], width, height);
        set_view_projection(vFullscreenLight, &camera[i], width, height);
        set_view_projection(vLight, &camera[i], width, height);
        set_view_projection(vTransparent, &camera[i], width, height);
    }
}

static void DrawOpaqueMeshes(ecs_iter_t *it) {

    FrameData        *frame_data        = ecs_field(it, FrameData, 1);
    PBRShader        *pbr_shader        = ecs_field(it, PBRShader, 2);
    DeferredRenderer *deferred_renderer = ecs_field(it, DeferredRenderer, 3);

    ecs_iter_t components_iterator = ecs_query_iter(it->world, it->ctx);

    uint64_t state = BGFX_STATE_DEFAULT & ~BGFX_STATE_CULL_MASK;

    while (ecs_query_next(&components_iterator)) {

        Mesh      *mesh      = ecs_field(&components_iterator, Mesh, 1);
        Material  *material  = ecs_field(&components_iterator, Material, 2);
        Transform *transform = ecs_field(&components_iterator, Transform, 3);

        for (int i = 0; i < components_iterator.count; i++) {

            // transparent materials are rendered in a separate forward pass
            // (view vTransparent)
            if (!material[i].blend) {
                for (size_t j = 0; j < ecs_vector_count(mesh[i].groups); j++) {
                    Group *group = ecs_vector_get(mesh[i].groups, Group, j);
                    bgfx_set_transform(&transform[i].value, 1);
                    set_normal_matrix(frame_data, transform[i].value);

                    bgfx_set_vertex_buffer(0, group->vertex_buffer, 0, UINT32_MAX);
                    bgfx_set_index_buffer(group->index_buffer, 0, UINT32_MAX);

                    uint64_t materialState = bind_material(pbr_shader, &material[i]);
                    bgfx_set_state(state | materialState, 0);

                    bgfx_submit(vGeometry, deferred_renderer->geometry_program, 0,
                                ~BGFX_DISCARD_TRANSFORM);
                }
            }
        }
    }

    // copy G-Buffer depth attachment to depth texture for sampling in the light
    // pass we can't attach it to the frame buffer and read it in the shader
    // (unprojecting world position) at the same time blit happens before any
    // compute or draw calls
    bgfx_blit(vFullscreenLight, deferred_renderer->light_depth_texture, 0, 0, 0, 0,
              bgfx_get_texture(deferred_renderer->g_buffer, G_Depth), 0, 0, 0, 0, UINT16_MAX,
              UINT16_MAX, UINT16_MAX);

    // bind these once for all following submits
    // excluding BGFX_DISCARD_TEXTURE_SAMPLERS from the discard flags passed to
    // submit makes sure they don't get unbound
    bind_g_buffer(deferred_renderer);
}

static void DrawPointLights(ecs_iter_t *it) {

    FrameData        *frame_data        = ecs_field(it, FrameData, 1);
    PBRShader        *pbr_shader        = ecs_field(it, PBRShader, 2);
    DeferredRenderer *deferred_renderer = ecs_field(it, DeferredRenderer, 3);

    // sun light + ambient light + emissive

    // full screen triangle
    // could also attach the accumulation buffer as a render target and write
    // out during the geometry pass this is a bit cleaner

    // full screen triangle, moved to far plane in the shader
    bgfx_set_vertex_buffer(0, frame_data->blit_triangle_buffer, 0, UINT32_MAX);
    bgfx_set_state(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_CULL_CW, 0);
    bgfx_submit(vFullscreenLight, deferred_renderer->fullscreen_program, 0, ~BGFX_DISCARD_BINDINGS);

    // point lights

    // render lights to framebuffer
    // cull with light geometry
    //   - axis-aligned bounding box (TODO? sphere for point lights)
    //   - read depth from geometry pass
    //   - reverse depth test
    //   - render backfaces
    //   - this shades all pixels between camera and backfaces
    // accumulate light contributions (blend mode add)
    // TODO? tiled-deferred is probably faster for small lights
    // https://software.intel.com/sites/default/files/m/d/4/1/d/8/lauritzen_deferred_shading_siggraph_2010.pdf

    bgfx_set_vertex_buffer(0, deferred_renderer->point_light_vertex_buffer, 0, UINT32_MAX);
    bgfx_set_index_buffer(deferred_renderer->point_light_index_buffer, 0, UINT32_MAX);

    ecs_iter_t components_iterator = ecs_query_iter(it->world, it->ctx);
    while (ecs_query_next(&components_iterator)) {

        PointLight *point_light = ecs_field(&components_iterator, PointLight, 1);

        for (int i = 0; i < components_iterator.count; i++) {
            // position light geometry (bounding box)
            // TODO if the light extends past the far plane, it won't get
            // rendered
            // - clip light extents to not extend past far plane
            // - use screen aligned quads (how to test depth?)
            // - tiled-deferred
            PointLight light  = point_light[i];
            float      radius = calculate_point_light_radius(&light);
            mat4       scale  = GLM_MAT4_IDENTITY_INIT;
            glm_scale(scale, (vec3){radius, radius, radius});
            mat4 translate = GLM_MAT4_IDENTITY_INIT;
            glm_translate(translate, light.position);
            mat4 model;
            glm_mat4_mul(translate, scale, model);

            bgfx_set_transform(model, 1);
            float lightIndexVec[4] = {(float)i};
            bgfx_set_uniform(deferred_renderer->light_index_vec_uniform, lightIndexVec, UINT16_MAX);
            bgfx_set_state(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GEQUAL |
                               BGFX_STATE_CULL_CCW | BGFX_STATE_BLEND_ADD,
                           0);
            bgfx_submit(
                vLight, deferred_renderer->point_light_program, 0,
                ~(BGFX_DISCARD_VERTEX_STREAMS | BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_BINDINGS));
        }
    }
}

static void DrawTransparentMeshes(ecs_iter_t *it) {

    FrameData        *frame_data        = ecs_field(it, FrameData, 1);
    PBRShader        *pbr_shader        = ecs_field(it, PBRShader, 2);
    DeferredRenderer *deferred_renderer = ecs_field(it, DeferredRenderer, 3);

    ecs_iter_t components_iterator = ecs_query_iter(it->world, it->ctx);

    uint64_t state = BGFX_STATE_DEFAULT & ~BGFX_STATE_CULL_MASK;

    while (ecs_query_next(&components_iterator)) {

        Mesh      *mesh      = ecs_field(&components_iterator, Mesh, 1);
        Material  *material  = ecs_field(&components_iterator, Material, 2);
        Transform *transform = ecs_field(&components_iterator, Transform, 3);

        for (int i = 0; i < components_iterator.count; i++) {

            // transparent materials are rendered in a separate forward pass
            // (view vTransparent)
            if (material[i].blend) {
                for (size_t j = 0; j < ecs_vector_count(mesh[i].groups); j++) {
                    Group *group = ecs_vector_get(mesh[i].groups, Group, j);

                    bgfx_set_transform(&transform[i].value, 1);
                    set_normal_matrix(frame_data, transform[i].value);

                    bgfx_set_vertex_buffer(0, group->vertex_buffer, 0, UINT32_MAX);
                    bgfx_set_index_buffer(group->index_buffer, 0, UINT32_MAX);

                    uint64_t materialState = bind_material(pbr_shader, &material[i]);
                    bgfx_set_state(state | materialState, 0);

                    bgfx_submit(vTransparent, deferred_renderer->transparency_program, 0,
                                ~BGFX_DISCARD_BINDINGS | BGFX_DISCARD_INDEX_BUFFER |
                                    BGFX_DISCARD_VERTEX_STREAMS);
                }
            }
        }
    }

    bgfx_discard(BGFX_DISCARD_ALL);
}

// Frame flow:
// * base_rendering_system Update
// * forward_renderer_system Update
// * projection
// * pbr bind albedo
// * bind ligts
// * mesh submit system
// * blit to screen
void DeferredRendererSystemImport(world_t *world) {
    ECS_TAG(world, OnBeginRender);
    ECS_TAG(world, OnRender);
    ECS_MODULE(world, DeferredRendererSystem);

    ECS_IMPORT(world, RendererComponents);
    ECS_IMPORT(world, SceneComponents);
    ECS_IMPORT(world, GuiComponents);

    ECS_IMPORT(world, BaseRenderingSystem);
    ECS_IMPORT(world, CameraSystem);
    ECS_IMPORT(world, PBRSystem);
    ECS_IMPORT(world, LightSystem);
    ECS_IMPORT(world, BgfxSystem);

    ECS_OBSERVER(world, InitializeDeferredRenderer, EcsOnSet, [in] bgfx.components.Bgfx);

    ECS_OBSERVER(world, OnAppWindowResized,
                 EcsOnSet, [in] gui.components.AppWindow, [in] bgfx.components.Bgfx);

    ctx = ecs_query_new(world, "FrameData, DeferredRenderer");

    ECS_SYSTEM(
        world, DeferredRendererBeginFrame,
        OnBeginRender, [in] renderer.components.DeferredRenderer, [in] gui.components.AppWindow,
        renderer.components.FrameData, [in] scene.components.Camera);

    ECS_SYSTEM(world, DrawOpaqueMeshes, OnBeginRender, renderer.components.FrameData,
               renderer.components.PBRShader, renderer.components.DeferredRenderer);
    ecs_system(world, {.entity = DrawOpaqueMeshes,
                       .ctx    = ecs_query_new(world, "Mesh, Material, Transform")});

    ECS_SYSTEM(world, DrawPointLights, OnRender, renderer.components.FrameData,
               renderer.components.PBRShader, renderer.components.DeferredRenderer);
    ecs_system(world, {.entity = DrawPointLights, .ctx = ecs_query_new(world, "PointLight")});

    ECS_SYSTEM(world, DrawTransparentMeshes, OnRender, renderer.components.FrameData,
               renderer.components.PBRShader, renderer.components.DeferredRenderer);
    ecs_system(world, {.entity = DrawTransparentMeshes,
                       .ctx    = ecs_query_new(world, "Mesh, Material, Transform")});
}