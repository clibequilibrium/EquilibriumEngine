
#include "base_rendering_system.h"
#include "bgfx_system.h"
#include "components/gui.h"
#include "utils/bgfx_utils.h"

bgfx_texture_format_t find_depth_format(uint64_t textureFlags, bool stencil) {
    const bgfx_texture_format_t depthFormats[] = {BGFX_TEXTURE_FORMAT_D16, BGFX_TEXTURE_FORMAT_D32};
    const bgfx_texture_format_t depthStencilFormats[] = {BGFX_TEXTURE_FORMAT_D24S8};
    const bgfx_texture_format_t *formats = stencil ? depthStencilFormats : depthFormats;
    size_t count = stencil ? BX_COUNT_OF(depthStencilFormats) : BX_COUNT_OF(depthFormats);

    bgfx_texture_format_t depthFormat = BGFX_TEXTURE_FORMAT_COUNT;
    for (size_t i = 0; i < count; i++) {
        if (bgfx_is_texture_valid(0, false, 1, formats[i], textureFlags)) {
            depthFormat = formats[i];
            break;
        }
    }

    ecs_assert(depthFormat != BGFX_TEXTURE_FORMAT_COUNT, ECS_INVALID_PARAMETER, NULL);

    return depthFormat;
}

static bgfx_frame_buffer_handle_t create_frame_buffer(world_t *world, bool hdr, bool depth) {
    bgfx_texture_handle_t textures[2];
    uint8_t               attachments = 0;

    uint64_t samplerFlags = BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT |
                            BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

    if (ecs_count(world, ForwardRenderer)) {
        samplerFlags |= BGFX_TEXTURE_RT_MSAA_X16;
    }

    bgfx_texture_format_t format =
        hdr ? BGFX_TEXTURE_FORMAT_RGBA16F : BGFX_TEXTURE_FORMAT_BGRA8; // BGRA is often faster
                                                                       // (internal GPU format)
    ecs_assert(bgfx_is_texture_valid(0, false, 1, format, BGFX_TEXTURE_RT | samplerFlags) == true,
               ECS_INVALID_PARAMETER, NULL);
    textures[attachments++] = create_texture_2d_scaled(world, BGFX_BACKBUFFER_RATIO_EQUAL, false, 1,
                                                       format, BGFX_TEXTURE_RT | samplerFlags);

    if (depth) {
        bgfx_texture_format_t depthFormat =
            find_depth_format(BGFX_TEXTURE_RT_WRITE_ONLY | samplerFlags, false);
        ecs_assert(depthFormat != BGFX_TEXTURE_FORMAT_COUNT, ECS_INVALID_PARAMETER, NULL);
        textures[attachments++] =
            create_texture_2d_scaled(world, BGFX_BACKBUFFER_RATIO_EQUAL, false, 1, depthFormat,
                                     BGFX_TEXTURE_RT_WRITE_ONLY | samplerFlags);
    }

    bgfx_frame_buffer_handle_t fb = create_frame_buffer_from_handles(world, attachments, textures);

    if (!BGFX_HANDLE_IS_VALID(fb))
        ecs_warn("Failed to create framebuffer");

    return fb;
}

static void InitializeFrameData(ecs_iter_t *it) {

    entity_t   entity     = (entity_t){it->entities[0], it->world};
    FrameData *frame_data = entity_get_or_add_component(entity, FrameData);

    bgfx_vertex_layout_t pcvDecl;

    bgfx_vertex_layout_begin(&pcvDecl, bgfx_get_renderer_type());
    bgfx_vertex_layout_add(&pcvDecl, BGFX_ATTRIB_POSITION, 3, BGFX_ATTRIB_TYPE_FLOAT, false, false);
    bgfx_vertex_layout_end(&pcvDecl);

    frame_data->blit_sampler = create_uniform(it->world, "s_texColor", BGFX_UNIFORM_TYPE_SAMPLER);
    frame_data->cam_pos_uniform = create_uniform(it->world, "u_camPos", BGFX_UNIFORM_TYPE_VEC4);
    frame_data->normal_matrix_uniform =
        create_uniform(it->world, "u_normalMatrix", BGFX_UNIFORM_TYPE_MAT3);
    frame_data->exposure_vec_uniform =
        create_uniform(it->world, "u_exposureVec", BGFX_UNIFORM_TYPE_VEC4);
    frame_data->tonemapping_mode_vec_uniform =
        create_uniform(it->world, "u_tonemappingModeVec", BGFX_UNIFORM_TYPE_VEC4);

    // triangle used for blitting
    const float     BOTTOM = -1.0f, TOP = 3.0f, LEFT = -1.0f, RIGHT = 3.0f;
    const PosVertex vertices[3] = {{LEFT, BOTTOM, 0.0f}, {RIGHT, BOTTOM, 0.0f}, {LEFT, TOP, 0.0f}};
    frame_data->blit_triangle_buffer = create_vertex_buffer(
        it->world, bgfx_copy(&vertices, sizeof(vertices)), &pcvDecl, BGFX_BUFFER_NONE);

    frame_data->blit_program =
        create_program(entity, blit_program, FrameData, "vs_tonemap.bin", "fs_tonemap.bin");

    const AppWindow *app_window = ecs_get(it->world, it->entities[0], AppWindow);
    frame_data->frame_buffer    = create_frame_buffer(it->world, true, true);
    bgfx_set_frame_buffer_name(frame_data->frame_buffer, "Render framebuffer (pre-postprocessing)",
                               INT32_MAX);

    ecs_trace("Base rendering system initialized");
}

static void OnBaseRendererUpdate(ecs_iter_t *it) {

    FrameData *frame_data = ecs_field(it, FrameData, 1);

    for (int i = 0; i < it->count; i++) {

        ecs_iter_t camera_iterator = ecs_query_iter(it->world, it->ctx);
        while (ecs_query_next(&camera_iterator)) {
            Camera *camera = ecs_field(&camera_iterator, Camera, 1);

            for (int j = 0; j < camera_iterator.count; j++) {

                vec4 camPos = {camera[j].position[0], camera[j].position[1], camera[j].position[2],
                               1.0f};
                bgfx_set_uniform(frame_data[i].cam_pos_uniform, &camPos[0], UINT16_MAX);

                // glm::vec3 linear =
                //     pbr.whiteFurnaceEnabled
                //         ? glm::vec3(PBRShader::WHITE_FURNACE_RADIANCE)
                //         : glm::convertSRGBToLinear(scene->skyColor); //
                //         tonemapping expects linear colors
                // glm::u8vec3 result =
                // glm::u8vec3(glm::round(glm::clamp(linear, 0.0f, 1.0f) *
                // 255.0f)); clearColor = (result[0] << 24) | (result[1] << 16)
                // | (result[2] << 8) | 255;
            }
        }
    }
}

static void BlitToScreen(ecs_iter_t *it) {

    const int MAX_VIEW = 254;

    AppWindow *app_window = ecs_field(it, AppWindow, 1);
    FrameData *frame_data = ecs_field(it, FrameData, 2);

    for (int i = 0; i < it->count; i++) {

        int view = MAX_VIEW;

        bgfx_set_view_name(view, "Tonemapping");
        bgfx_set_view_clear(view, BGFX_CLEAR_NONE, 255U, 1.0F, 0U);
        bgfx_set_view_rect(view, 0, 0, app_window[i].width, app_window[i].height);

        bgfx_frame_buffer_handle_t invalid_handle = BGFX_INVALID_HANDLE;
        bgfx_set_view_frame_buffer(view, invalid_handle);
        bgfx_set_state(BGFX_STATE_WRITE_RGB | BGFX_STATE_CULL_CW, 0);

        bgfx_texture_handle_t frame_buffer_handle = bgfx_get_texture(frame_data[i].frame_buffer, 0);
        bgfx_set_texture(0, frame_data[i].blit_sampler, frame_buffer_handle, UINT32_MAX);
        // float exposureVec[4] = {scene->loaded ? scene->camera.exposure
        // : 1.0f};
        float exposure_vec[4] = {1.0f};
        bgfx_set_uniform(frame_data[i].exposure_vec_uniform, &exposure_vec[0], UINT16_MAX);
        float tonemapping_mode_vec[4] = {(float)7};

        //  enum class TonemappingMode : int
        //     {
        //         NONE = 0,
        //         EXPONENTIAL,
        //         REINHARD,
        //         REINHARD_LUM,
        //         HABLE,
        //         DUIKER,
        //         ACES,
        //         ACES_LUM
        //     };

        bgfx_set_uniform(frame_data[i].tonemapping_mode_vec_uniform, &tonemapping_mode_vec[0],
                         UINT16_MAX);
        bgfx_set_vertex_buffer(0, frame_data[i].blit_triangle_buffer, 0, UINT32_MAX);

        bgfx_submit(view, frame_data[i].blit_program, 0, BGFX_DISCARD_ALL);
    }
}

void BaseRenderingSystemImport(world_t *world) {
    ECS_TAG(world, OnBeginRender);
    ECS_TAG(world, OnEndRender);

    ECS_MODULE(world, BaseRenderingSystem);
    ECS_IMPORT(world, RendererComponents);

    ECS_OBSERVER(world, InitializeFrameData, EcsOnSet, renderer.components.FrameData);
    ECS_SYSTEM(world, OnBaseRendererUpdate, OnBeginRender, renderer.components.FrameData);
    ecs_system(world, {.entity = OnBaseRendererUpdate, .ctx = ecs_query_new(world, "Camera")});

    ECS_SYSTEM(world, BlitToScreen, OnEndRender, [in] gui.components.AppWindow,
               renderer.components.FrameData);
}