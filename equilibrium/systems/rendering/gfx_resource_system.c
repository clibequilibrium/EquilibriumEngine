
#include "gfx_resource_system.h"
#include "utils/bgfx_utils.h"
#include <bgfx/c99/bgfx.h>
#include <x-watcher.h>
#include <cr.h>

ECS_COMPONENT_DECLARE(GfxResource);
ECS_COMPONENT_DECLARE(HotReloadableShader);
ECS_COMPONENT_DECLARE(FileWatcher);
ECS_COMPONENT_DECLARE(ReloadShader);

static void ReplaceSlashes(char *path, char character) {
    int index = 0;
    while (path[index]) {
        if (path[index] == character)
            path[index] = '\\';
        index++;
    }
}

void callback_func(XWATCHER_FILE_EVENT event, const char *path, int context, void *data) {
    if (event == XWATCHER_FILE_MODIFIED) {
        // ecs_trace("Hot reloading shader: %s ", path);

        entity_t shader_entity = (entity_t){context, data};

        // const HotReloadableShader *shader =
        //     ecs_get(shader_entity.world, shader_entity.handle,
        //     HotReloadableShader);

        // TODO: optimize to have only 1 handle reloaded
        // if (strcmp(shader->vertex_shader_name, path) == 0) {
        //   reload_shader.type = Vertex;
        // }

        // if (strcmp(shader->fragment_shader_name, path) == 0) {
        //   reload_shader.type = Fragment;
        // }

        ecs_set(shader_entity.world, shader_entity.handle, ReloadShader, {});
    }
}

static void DestroyGfxResources(ecs_iter_t *it) {
    GfxResource *resources = ecs_field(it, GfxResource, 1);

    for (size_t i = 0; i < it->count; i++) {

        GfxResource resource = resources[i];
        if (resource.handle == UINT16_MAX) {
            ecs_err("Invalid GfxResource handle");
            continue;
        }

        switch (resources[i].type) {
        case RESOURCE_TYPE_TEXTURE:
            bgfx_destroy_texture((bgfx_texture_handle_t){resources[i].handle});
            break;
        case RESOURCE_TYPE_VERTEX_BUFFER:
            bgfx_destroy_vertex_buffer((bgfx_vertex_buffer_handle_t){resources[i].handle});
            break;
        case RESOURCE_TYPE_DYNAMIC_VERTEX_BUFFER:
            bgfx_destroy_dynamic_vertex_buffer(
                (bgfx_dynamic_vertex_buffer_handle_t){resources[i].handle});
            break;
        case RESOURCE_TYPE_INDEX_BUFFER:
            bgfx_destroy_index_buffer((bgfx_index_buffer_handle_t){resources[i].handle});
            break;
        case RESOURCE_TYPE_PROGRAM:
            bgfx_destroy_program((bgfx_program_handle_t){resources[i].handle});
            break;
        case RESOURCE_TYPE_FRAME_BUFFER:
            bgfx_destroy_frame_buffer((bgfx_frame_buffer_handle_t){resources[i].handle});
            break;
        case RESOURCE_TYPE_UNIFORM:
            bgfx_destroy_uniform((bgfx_uniform_handle_t){resources[i].handle});
            break;

        case RESOURCE_TYPE_INVALID:
        default:
            ecs_err("Unsupported resource type");
            break;
        }
    }
}

static void InitHotReloadShaders(ecs_iter_t *it) {

    FileWatcher         *watcher = ecs_field(it, FileWatcher, 1);
    HotReloadableShader *shaders = ecs_field(it, HotReloadableShader, 2);

    for (size_t i = 0; i < it->count; i++) {
        HotReloadableShader shader = shaders[i];
        ReplaceSlashes(shader.vertex_shader_name, '/');
        ReplaceSlashes(shader.fragment_shader_name, '/');

        xWatcher_reference vertex, fragment;
        vertex.path          = shader.vertex_shader_name;
        fragment.path        = shader.fragment_shader_name;
        vertex.callback_func = fragment.callback_func = callback_func;
        fragment.context = vertex.context = it->entities[i];
        fragment.additional_data = vertex.additional_data = it->world;

        xWatcher_appendFile(watcher->data, &vertex);
        xWatcher_appendFile(watcher->data, &fragment);
    }
}

static void DestroyHotReloadShaders(ecs_iter_t *it) {

    HotReloadableShader *shaders = ecs_field(it, HotReloadableShader, 1);

    for (size_t i = 0; i < it->count; i++) {
        HotReloadableShader shader = shaders[i];
        free(shader.vertex_shader_name);
        free(shader.fragment_shader_name);
    }
}

static void DestroyFileWatcher(ecs_iter_t *it) {
    FileWatcher *watcher = ecs_field(it, FileWatcher, 1);
    xWatcher_destroy(watcher->data);
}

static void HotReloadShaders(ecs_iter_t *it) {

    HotReloadableShader *shaders        = ecs_field(it, HotReloadableShader, 1);
    GfxResource         *gfx_resources  = ecs_field(it, GfxResource, 2);
    ReloadShader        *reload_shaders = ecs_field(it, ReloadShader, 3);

    for (size_t i = 0; i < it->count; i++) {
        entity_t entity = {it->entities[i], it->world};

        HotReloadableShader shader        = shaders[i];
        GfxResource         resource      = gfx_resources[i];
        ReloadShader        reload_shader = reload_shaders[i];

        ShaderHandle vertex_handle   = shader_load(entity.world, shader.vertex_shader_name, true);
        ShaderHandle fragment_handle = shader_load(entity.world, shader.fragment_shader_name, true);

        if (!BGFX_HANDLE_IS_VALID(vertex_handle.handle) ||
            !BGFX_HANDLE_IS_VALID(fragment_handle.handle))
            continue;

        // Destroy old handle and create new one
        bgfx_destroy_program((bgfx_program_handle_t){resource.handle});

        resource.handle =
            bgfx_create_program(vertex_handle.handle, fragment_handle.handle, true).idx;

        // Assign the field to the owner of the program

        void *component = ecs_get_mut_id(shader.shader_entity.world, shader.shader_entity.handle,
                                         shader.component_id);

        *(bgfx_program_handle_t *)(component + shader.filed_offset) =
            (bgfx_program_handle_t){resource.handle};

        // ecs_set_id(it->world, shader.shader_entity.handle,
        // shader.component_id, shader.component_size,
        //            component);

        gfx_resources[i] = resource;
        shaders[i]       = shader;

        ecs_trace("Hot reload successful for %s | %s shader program: %s", shader.vertex_shader_name,
                  shader.fragment_shader_name, entity_get_name(entity));
    }
}

// observer file updates on main thread, TODO: offload on worker? world deffer
// etc
static void UpdateFileWatcher(ecs_iter_t *it) {
    FileWatcher *watcher = ecs_field(it, FileWatcher, 1);
    xWatcherUpdate(watcher->data);
}

// TODO xWatcher remove extra thread to avoid having more threads than 2x cores
void GfxResourceSystemImport(world_t *world) {
    ECS_TAG(world, OnInput)
    ECS_MODULE(world, GfxResourceSystem);

    ECS_COMPONENT_DEFINE(world, GfxResource);
    ECS_COMPONENT_DEFINE(world, HotReloadableShader);
    ECS_COMPONENT_DEFINE(world, FileWatcher);
    ECS_COMPONENT_DEFINE(world, ReloadShader);

    FileWatcher watcher = {xWatcher_create()};
    ecs_set_ptr(world, ecs_id(FileWatcher), FileWatcher, &watcher);

    ECS_OBSERVER(world, DestroyGfxResources, EcsUnSet, GfxResource);
    ECS_OBSERVER(world, DestroyHotReloadShaders, EcsUnSet, HotReloadableShader);
    ECS_OBSERVER(world, DestroyFileWatcher, EcsUnSet, FileWatcher($));

    ECS_OBSERVER(world, InitHotReloadShaders, EcsOnSet, FileWatcher($), HotReloadableShader);
    ECS_OBSERVER(world, HotReloadShaders, EcsOnSet, HotReloadableShader, GfxResource, ReloadShader);

    ECS_SYSTEM(world, UpdateFileWatcher, OnInput, FileWatcher($));
}