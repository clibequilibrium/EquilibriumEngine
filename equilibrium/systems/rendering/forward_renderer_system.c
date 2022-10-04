
#include "forward_renderer_system.h"
#include "base_rendering_system.h"
#include "pbr_system.h"
#include "light_system.h"
#include "bgfx_system.h"
#include "scene/camera_system.h"
#include "components/renderer/renderer_components.h"
#include "components/gui.h"
#include "utils/bgfx_utils.h"

static bgfx_view_id_t default_view = 0;

static void InitializeForwardRenderer(ecs_iter_t *it) {

    if (!renderer_supported(false)) {
        ecs_err("Forward rendering is not supported on this device");
        return;
    }

    entity_t         entity           = (entity_t){it->entities[0], it->world};
    ForwardRenderer *forward_renderer = entity_get_or_add_component(entity, ForwardRenderer);

    forward_renderer->program =
        create_program(entity, program, ForwardRenderer, "vs_forward.bin", "fs_forward.bin");
    ecs_set(it->world, it->entities[0], FrameData, {.frame_buffer = BGFX_INVALID_HANDLE});
    ecs_set(it->world, it->entities[0], PBRShader, {.albedo_lut_program = BGFX_INVALID_HANDLE});
    ecs_set(it->world, it->entities[0], LightShader,
            {.light_count_vec_uniform = BGFX_INVALID_HANDLE});

    ecs_trace("Forward rendering System initialized");
}

static void ForwardRendererBeginFrame(ecs_iter_t *it) {

    ForwardRenderer *forward_renderer = ecs_field(it, ForwardRenderer, 1);
    AppWindow       *app_window       = ecs_field(it, AppWindow, 2);
    FrameData       *frame_data       = ecs_field(it, FrameData, 3);
    Camera          *camera           = ecs_field(it, Camera, 4);

    for (int i = 0; i < it->count; i++) {

        if (!BGFX_HANDLE_IS_VALID(frame_data[i].frame_buffer))
            continue;

        bgfx_set_view_name(default_view, "Forward render pass");
        bgfx_set_view_clear(default_view, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030FF, 1.0f, 0);
        bgfx_set_view_rect(default_view, 0, 0, app_window[i].width, app_window[i].height);
        bgfx_set_view_frame_buffer(default_view, frame_data[i].frame_buffer);

        bgfx_touch(default_view);
        set_view_projection(default_view, &camera[i], app_window[i].width, app_window[i].height);
    }
}

static void DrawMeshes(ecs_iter_t *it) {

    FrameData       *frame_data       = ecs_field(it, FrameData, 1);
    PBRShader       *pbr_shader       = ecs_field(it, PBRShader, 2);
    ForwardRenderer *forward_renderer = ecs_field(it, ForwardRenderer, 3);

    ecs_iter_t components_iterator = ecs_query_iter(it->world, it->ctx);
    uint64_t   state               = BGFX_STATE_DEFAULT & ~BGFX_STATE_CULL_MASK;

    bool rendered = false;
    while (ecs_query_next(&components_iterator)) {

        Mesh      *mesh      = ecs_field(&components_iterator, Mesh, 1);
        Material  *material  = ecs_field(&components_iterator, Material, 2);
        Transform *transform = ecs_field(&components_iterator, Transform, 3);

        for (int i = 0; i < components_iterator.count; i++) {
            for (size_t j = 0; j < ecs_vector_count(mesh[i].groups); j++) {
                Group *group = ecs_vector_get(mesh[i].groups, Group, j);

                bgfx_set_transform(&transform[i].value, 1);
                set_normal_matrix(frame_data, transform[i].value);

                bgfx_set_vertex_buffer(0, group->vertex_buffer, 0, UINT32_MAX);
                bgfx_set_index_buffer(group->index_buffer, 0, UINT32_MAX);

                uint64_t materialState = bind_material(pbr_shader, &material[i]);
                bgfx_set_state(state | materialState, 0);

                bgfx_submit(default_view, forward_renderer->program, 0,
                            ~BGFX_DISCARD_BINDINGS | BGFX_DISCARD_INDEX_BUFFER |
                                BGFX_DISCARD_VERTEX_STREAMS);
            }

            rendered = true;
        }
    }

    if (rendered) {
        bgfx_discard(BGFX_DISCARD_ALL);
    }
}

// Frame flow:
// * base_rendering_system Update
// * forward_renderer_system Update
// * projection
// * pbr bind albedo
// * bind ligts
// * mesh submit system
// * blit to screen
void ForwardRendererSystemImport(world_t *world) {
    ECS_TAG(world, OnBeginRender);
    ECS_TAG(world, OnRender);
    ECS_MODULE(world, ForwardRendererSystem);

    ECS_IMPORT(world, RendererComponents);
    ECS_IMPORT(world, SceneComponents);
    ECS_IMPORT(world, GuiComponents);

    ECS_IMPORT(world, BaseRenderingSystem);
    ECS_IMPORT(world, CameraSystem);
    ECS_IMPORT(world, PBRSystem);
    ECS_IMPORT(world, LightSystem);
    ECS_IMPORT(world, BgfxSystem);

    ECS_OBSERVER(world, InitializeForwardRenderer, EcsOnSet, [in] bgfx.components.Bgfx);

    ECS_SYSTEM(
        world, ForwardRendererBeginFrame,
        OnBeginRender, [in] renderer.components.ForwardRenderer, [in] gui.components.AppWindow,
        renderer.components.FrameData, [in] scene.components.Camera);

    ECS_SYSTEM(world, DrawMeshes, OnRender, renderer.components.FrameData,
               renderer.components.PBRShader, renderer.components.ForwardRenderer);
    ecs_system(world,
               {.entity = DrawMeshes, .ctx = ecs_query_new(world, "Mesh, Material, Transform")});
}