#ifndef ENTITY_H
#define ENTITY_H

#include <flecs.h>
#include "equilibrium_defines.h"

EQUILIBRIUM_API extern ECS_DECLARE(Entity);

typedef struct entity_t {
    ecs_entity_t handle;
    void        *world;
} entity_t;

EQUILIBRIUM_API
entity_t entity_create_empty(void *world, const char *name);

EQUILIBRIUM_API
void entity_destroy(entity_t entity);

EQUILIBRIUM_API
bool entity_valid(entity_t entity);

#define entity_create(ecs_world, name, component, ...)                                             \
    ({                                                                                             \
        entity_t entity = (entity_t){ecs_new(ecs_world, 0), ecs_world};                            \
        ecs_doc_set_name(entity.world, entity.handle,                                              \
                         name == NULL || name[0] == '\0' ? "Entity" : name);                       \
        ecs_set(entity.world, entity.handle, component, __VA_ARGS__);                              \
        ecs_add(entity.world, entity.handle, Entity);                                              \
        entity;                                                                                    \
    })

#define entity_add_component(entity, component, ...)                                               \
    ecs_set(entity.world, entity.handle, component, __VA_ARGS__)

#define entity_get_or_add_component(entity, T)    ({ ecs_get_mut(entity.world, entity.handle, T); })

#define entity_get_name(entity)                   ecs_doc_get_name(entity.world, entity.handle)
#define entity_get_read_only_component(entity, T) ecs_get(entity.world, entity.handle, T)
#define entity_has_component(entity, T)           ecs_has(entity.world, entity.handle, T)
#define entity_remove_component(entity, T)        ecs_remove(entity.world, entity.handle, T)
#define entity_remove_component_id(entity, id)    ecs_remove_id(entity.world, entity.handle, id)

#endif