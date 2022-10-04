#include "entity.h"

ECS_DECLARE(Entity);

entity_t entity_create_empty(void *world, const char *name) {
    entity_t entity = (entity_t){ecs_new(world, 0), world};
    ecs_doc_set_name(world, entity.handle, name == NULL || name[0] == '\0' ? "Entity" : name);
    ecs_add(entity.world, entity.handle, Entity);
    return entity;
}

void entity_destroy(entity_t entity) { ecs_delete(entity.world, entity.handle); }

bool entity_valid(entity_t entity) {
    return entity.world != NULL && ecs_is_valid(entity.world, entity.handle);
}