#ifndef GFX_RESOURCE_SYSTEM_H
#define GFX_RESOURCE_SYSTEM_H

#include "base.h"

// TODO naming conventions for ENUMS

typedef enum ResourceType {
    RESOURCE_TYPE_INVALID,
    RESOURCE_TYPE_TEXTURE,
    RESOURCE_TYPE_VERTEX_BUFFER,
    RESOURCE_TYPE_DYNAMIC_VERTEX_BUFFER,
    RESOURCE_TYPE_INDEX_BUFFER,
    RESOURCE_TYPE_PROGRAM,
    RESOURCE_TYPE_FRAME_BUFFER,
    RESOURCE_TYPE_UNIFORM,
} ResourceType;

typedef struct GfxResource {
    ResourceType type;
    uint16_t     handle;
} GfxResource;

typedef struct HotReloadableShader {
    entity_t   shader_entity;
    ecs_id_t   component_id;
    ecs_size_t component_size;
    int32_t    filed_offset;
    char      *vertex_shader_name;
    char      *fragment_shader_name;
} HotReloadableShader;

typedef struct ReloadShader {
    int32_t dummy;
} ReloadShader;

typedef struct FileWatcher {
    void *data;
} FileWatcher;

EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(GfxResource);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(HotReloadableShader);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(FileWatcher);
EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(ReloadShader);

EQUILIBRIUM_API
void GfxResourceSystemImport(world_t *world);

#endif