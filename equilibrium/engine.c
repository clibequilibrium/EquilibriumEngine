#include "engine.h"
#include "base.h"
#include "entity.h"
#include "flecs.h"
#include "logging.h"

static ecs_query_t *run_on_input;
static ecs_query_t *run_on_begin_render;
static ecs_query_t *run_on_render;
static ecs_query_t *run_on_end_render;

static int compare_entity(ecs_entity_t e1, const void *ptr1, ecs_entity_t e2, const void *ptr2) {
    (void)ptr1;
    (void)ptr2;
    return (e1 > e2) - (e1 < e2);
}

static inline ecs_query_t *query_create(world_t *world, const char *name) {
    return ecs_query_init(world, &(ecs_query_desc_t){
                                     .filter =
                                         {
                                             .expr = name,
                                         },
                                     .order_by = compare_entity,
                                 });
}

engine_t engine_init(int32_t num_threads, bool enable_rest) {
    ecs_log_set_level(0);
    ecs_log_enable_colors(true);

    engine_t engine;
    engine.world   = world_create();
    engine.running = true;

    ecs_os_api.log_ = custom_log;

    if (enable_rest) {
        ecs_set(engine.world, EcsWorld, EcsRest, {.port = 0});
    }

    ECS_TAG_DEFINE(engine.world, Entity);
    ECS_IMPORT(engine.world, FlecsMonitor);

    if (num_threads > 1)
        ecs_set_threads(engine.world, num_threads);

    ECS_TAG(engine.world, OnInput);
    ECS_TAG(engine.world, OnBeginRender);
    ECS_TAG(engine.world, OnRender);
    ECS_TAG(engine.world, OnEndRender);

    run_on_input        = query_create(engine.world, "OnInput");
    run_on_begin_render = query_create(engine.world, "OnBeginRender");
    run_on_render       = query_create(engine.world, "OnRender");
    run_on_end_render   = query_create(engine.world, "OnEndRender");

    return engine;
}

bool engine_update(engine_t *engine) {
    world_t *world = engine->world;

    if (!ecs_should_quit(world)) {

        const ecs_world_info_t *info = ecs_get_world_info(world);
        ecs_iter_t              it;

        if (run_on_input != NULL) {
            it = ecs_query_iter(world, run_on_input);
            while (ecs_query_next(&it)) {
                for (int i = 0; i < it.count; i++) {
                    ecs_run(world, it.entities[i], info->delta_time, NULL);
                }
            }
        }

        ecs_progress(world, 0);

        if (run_on_begin_render != NULL) {
            it = ecs_query_iter(world, run_on_begin_render);
            while (ecs_query_next(&it)) {
                for (int i = 0; i < it.count; i++) {
                    ecs_run(world, it.entities[i], info->delta_time, NULL);
                }
            }
        }

        if (run_on_render != NULL) {
            it = ecs_query_iter(world, run_on_render);
            while (ecs_query_next(&it)) {
                for (int i = 0; i < it.count; i++) {
                    ecs_run(world, it.entities[i], info->delta_time, NULL);
                }
            }
        }

        if (run_on_end_render != NULL) {
            it = ecs_query_iter(world, run_on_end_render);
            while (ecs_query_next(&it)) {
                for (int i = 0; i < it.count; i++) {
                    ecs_run(world, it.entities[i], info->delta_time, NULL);
                }
            }
        }

        return true;
    }

    ecs_fini(world);
    engine->running = false;

    return false;
}