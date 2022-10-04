#include "components/transform.h"
#include "transform_system.h"

void AddTransform(ecs_iter_t *it) {
    ecs_world_t *world = it->world;
    ecs_entity_t comp  = ecs_field_id(it, 1);

    int i;
    for (i = 0; i < it->count; i++) {
        ecs_add_id(world, it->entities[i], comp);
    }
}

void ApplyTransform(ecs_iter_t *it) {
    if (!ecs_query_changed(NULL, it)) {
        ecs_query_skip(it);
        return;
    }

    Transform *m        = ecs_field(it, Transform, 1);
    Transform *m_parent = ecs_field(it, Transform, 2);
    Position  *p        = ecs_field(it, Position, 3);
    Rotation  *r        = ecs_field(it, Rotation, 4);
    Scale     *s        = ecs_field(it, Scale, 5);
    int        i;

    if (!m_parent) {
        if (ecs_field_is_self(it, 3)) {
            for (i = 0; i < it->count; i++) {
                glm_translate_make(m[i].value, *(vec3 *)&p[i]);
            }
        } else {
            for (i = 0; i < it->count; i++) {
                glm_translate_make(m[i].value, *(vec3 *)p);
            }
        }
    } else {
        if (ecs_field_is_self(it, 3)) {
            for (i = 0; i < it->count; i++) {
                glm_translate_to(m_parent[0].value, *(vec3 *)&p[i], m[i].value);
            }
        } else {
            for (i = 0; i < it->count; i++) {
                glm_translate_to(m_parent[0].value, *(vec3 *)p, m[i].value);
            }
        }
    }

    if (r) {
        if (ecs_field_is_self(it, 4)) {
            for (i = 0; i < it->count; i++) {
                glm_rotate(m[i].value, r[i].x, (vec3){1.0, 0.0, 0.0});
                glm_rotate(m[i].value, r[i].y, (vec3){0.0, 1.0, 0.0});
                glm_rotate(m[i].value, r[i].z, (vec3){0.0, 0.0, 1.0});
            }
        } else {
            for (i = 0; i < it->count; i++) {
                glm_rotate(m[i].value, r->x, (vec3){1.0, 0.0, 0.0});
                glm_rotate(m[i].value, r->y, (vec3){0.0, 1.0, 0.0});
                glm_rotate(m[i].value, r->z, (vec3){0.0, 0.0, 1.0});
            }
        }
    }

    if (s) {
        for (i = 0; i < it->count; i++) {
            glm_scale(m[i].value, *(vec3 *)&s[i]);
        }
    }
}

void TransformSystemImport(world_t *world) {
    ECS_MODULE(world, TransformSystem);
    ECS_IMPORT(world, TransformComponents);

    /* System that adds transform matrix to every entity with transformations */
    ECS_SYSTEM(world, AddTransform, EcsPostLoad, [out] !transform.components.Transform,
               [filter] transform.components.Position(self|up) ||
                   [filter] transform.components.Rotation(self|up) ||
                   [filter] transform.components.Scale(self|up));

    ECS_SYSTEM(world, ApplyTransform, EcsOnValidate, 
        [out] transform.components.Transform,
        [in] ?transform.components.Transform(parent|cascade),
        [in] transform.components.Position(self|up),
        [in] ?transform.components.Rotation,
        [in] ?transform.components.Scale);

    ecs_system(world, {.entity = ApplyTransform, .query.filter.instanced = true});
}