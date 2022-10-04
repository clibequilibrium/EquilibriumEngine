#include <systems/imgui/cimgui_base.h>
#include "mouse_picking_system.h"
#include <components/gui.h>
#include <components/scene/scene_components.h>
#include <components/input.h>
#include <utils/bgfx_utils.h>
#include <cr.h>

#define RENDER_PASS_SHADING 0   // Default forward rendered geo with simple shading
#define RENDER_PASS_ID      125 // ID buffer for picking
#define RENDER_PASS_BLIT    126 // Blit GPU render target to CPU texture

ECS_COMPONENT_DECLARE(MousePickingData);

static void InitializeMousePicking(entity_t entity, MousePickingData *mouse_picking_data) {

    bgfx_set_view_clear(RENDER_PASS_BLIT, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff, 1.0f, 0);
    mouse_picking_data->tint = create_uniform(entity.world, "u_tint", BGFX_UNIFORM_TYPE_VEC4);
    mouse_picking_data->id   = create_uniform(entity.world, "u_id", BGFX_UNIFORM_TYPE_VEC4);

    mouse_picking_data->shading_program =
        create_program(entity, shading_program, MousePickingData, "vs_picking_shaded.bin",
                       "fs_picking_shaded.bin");

    mouse_picking_data->id_program = create_program(entity, id_program, MousePickingData,
                                                    "vs_picking_shaded.bin", "fs_picking_id.bin");

    mouse_picking_data->picking_rt =
        create_texture_2d(entity.world, ID_DIM, ID_DIM, false, 1, BGFX_TEXTURE_FORMAT_RGBA8,
                          0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT |
                              BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
                          NULL);

    mouse_picking_data->picking_rt_depth =
        create_texture_2d(entity.world, ID_DIM, ID_DIM, false, 1, BGFX_TEXTURE_FORMAT_D32F,
                          0 | BGFX_TEXTURE_RT | BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT |
                              BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
                          NULL);

    mouse_picking_data->blit_texture =
        create_texture_2d(entity.world, ID_DIM, ID_DIM, false, 1, BGFX_TEXTURE_FORMAT_RGBA8,
                          0 | BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK |
                              BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT |
                              BGFX_SAMPLER_MIP_POINT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
                          NULL);

    bgfx_texture_handle_t rt[2] = {mouse_picking_data->picking_rt,
                                   mouse_picking_data->picking_rt_depth

    };

    mouse_picking_data->picking_frame_buffer =
        create_frame_buffer_from_handles(entity.world, BX_COUNT_OF(rt), rt);
}

inline static ImTextureID toId(bgfx_texture_handle_t _handle, uint8_t _flags, uint8_t _mip) {
    union {
        struct {
            bgfx_texture_handle_t handle;
            uint8_t               flags;
            uint8_t               mip;
        } s;
        ImTextureID id;
    } tex;
    tex.s.handle = _handle;
    tex.s.flags  = _flags;
    tex.s.mip    = _mip;
    return tex.id;
}

static void UpdateMousePicking(ecs_iter_t *it) {

    MousePickingData *mouse_picking_data = ecs_field(it, MousePickingData, 1);
    Input            *input              = ecs_field(it, Input, 2);
    Camera           *camera             = ecs_field(it, Camera, 3);
    AppWindow        *app_window         = ecs_field(it, AppWindow, 4);

    const bgfx_caps_t *caps        = bgfx_get_caps();
    bool               blitSupport = 0 != (caps->supported & BGFX_CAPS_TEXTURE_BLIT);

    if (!blitSupport) {
        ecs_trace("BGFX_CAPS_TEXTURE_BLIT is not supported. \n");
        return;
    }

    bgfx_set_view_frame_buffer(RENDER_PASS_ID, mouse_picking_data->picking_frame_buffer);

    mat4 viewProj;
    glm_mat4_mul(camera->view, camera->proj, viewProj);

    mat4 invViewProj;
    glm_mat4_inv(viewProj, invViewProj);

    // Mouse coord in NDC
    float mouseXNDC = (input->mouse.wnd.x / (float)app_window->width) * 2.0f - 1.0f;
    float mouseYNDC =
        (((float)app_window->height - input->mouse.wnd.y) / (float)app_window->height) * 2.0f -
        1.0f;

    vec3 pickEye;
    mul_h((vec3){mouseXNDC, mouseYNDC, 0.0f}, (float *)invViewProj, pickEye);

    vec3 pickAt;
    mul_h((vec3){mouseXNDC, mouseYNDC, 1.0f}, (float *)invViewProj, pickAt);

    mat4 pickView;
    glm_look_anyup_lh(pickEye, pickAt, pickView);

    mat4 pickProj;
    glm_perspective(glm_rad(2.0f), 1, 0.1f, 100.0f, pickProj);

    // View rect and transforms for picking pass
    bgfx_set_view_rect(RENDER_PASS_ID, 0, 0, ID_DIM, ID_DIM);
    bgfx_set_view_transform(RENDER_PASS_ID, pickView, pickProj);

    bgfx_blit(RENDER_PASS_BLIT, mouse_picking_data->blit_texture, 0, 0, 0, 0,
              mouse_picking_data->picking_rt, 0, 0, 0, 0, UINT16_MAX, UINT16_MAX, UINT16_MAX);

    bgfx_read_texture(mouse_picking_data->blit_texture, mouse_picking_data->blid_data, 0);
}

void MousePickingSystemImport(world_t *world) {
    ECS_TAG(world, OnEndRender);

    ECS_MODULE(world, MousePickingSystem);
    ECS_IMPORT(world, GuiComponents);
    ECS_IMPORT(world, InputComponents);
    ECS_IMPORT(world, SceneComponents);

    ECS_COMPONENT_DEFINE(world, MousePickingData);

    entity_t singleton = {ecs_set_ptr(world, ecs_id(MousePickingData), MousePickingData, NULL),
                          world};

    InitializeMousePicking(singleton, entity_get_or_add_component(singleton, MousePickingData));

    ECS_SYSTEM(world, UpdateMousePicking, OnEndRender, mouse.picking.system.MousePickingData($),
               input.components.Input($), scene.components.Camera, gui.components.AppWindow);
}