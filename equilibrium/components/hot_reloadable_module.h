#ifndef HOT_RELOADABLE_MODULE_H
#define HOT_RELOADABLE_MODULE_H

#include "cr.h"
#include "base.h"
#include "engine.h"
#include "flecs.h"

typedef struct HotReloadableModule {
    char  module_name[128];
    void *plugin_ctx;
} HotReloadableModule;

EQUILIBRIUM_API extern ECS_COMPONENT_DECLARE(HotReloadableModule);

EQUILIBRIUM_API
void HotReloadableComponentsImport(world_t *world);

static ecs_query_t CR_STATE *query;

static void reload_modules(struct cr_plugin *ctx) {

    world_t *world = (world_t *)ctx->userdata;

    if (query)
        ecs_query_fini(query);

    query = ecs_query_init(world, &(ecs_query_desc_t){
                                      .filter =
                                          {
                                              .expr = "HotReloadableModule",
                                          },
                                  });

    if (query != NULL) {
        ecs_iter_t it = ecs_query_iter(world, query);
        while (ecs_query_next(&it)) {

            HotReloadableModule *modules = ecs_field(&it, HotReloadableModule, 1);

            for (int i = 0; i < it.count; i++) {

                if (modules[i].plugin_ctx != ctx)
                    continue;

                char        *path   = ecs_module_path_from_c(modules[i].module_name);
                ecs_entity_t entity = ecs_lookup_fullpath(world, path);

                if (entity != 0) {
                    ecs_delete(world, entity);
                }

                ecs_delete(world, it.entities[i]);
            }
        }
    }
}

#define CHECK_IF_PLUGIN_RELOADED(ctx, operation)                                                   \
    ({                                                                                             \
        bool     reloaded;                                                                         \
        world_t *world = (world_t *)ctx->userdata;                                                 \
        reloaded       = operation == CR_LOAD;                                                     \
        if (reloaded)                                                                              \
            reload_modules(ctx);                                                                   \
        reloaded;                                                                                  \
    })

#define CHECK_IF_PLUGIN_RELOADED_W_ACTION(ctx, operation, action)                                  \
    ({                                                                                             \
        bool     reloaded;                                                                         \
        world_t *world = (world_t *)ctx->userdata;                                                 \
        reloaded       = operation == CR_LOAD;                                                     \
        if (reloaded) {                                                                            \
            action(world);                                                                         \
            reload_modules(ctx);                                                                   \
        }                                                                                          \
        reloaded;                                                                                  \
    })

#define ECS_IMPORT_HOT_RELOADABLE(ctx, module)                                                     \
    ({                                                                                             \
        world_t     *world = (world_t *)ctx->userdata;                                             \
        char        *path  = ecs_module_path_from_c(#module);                                      \
        ecs_entity_t e     = ecs_lookup_fullpath(world, path);                                     \
        if (!e) {                                                                                  \
            ECS_IMPORT(world, module);                                                             \
            ECS_IMPORT(world, HotReloadableComponents);                                            \
            entity_t             entity = entity_create_empty(world, #module);                     \
            HotReloadableModule *module =                                                          \
                entity_get_or_add_component(entity, HotReloadableModule);                          \
            module->plugin_ctx = ctx;                                                              \
            strncpy_s(module->module_name, sizeof(module->module_name), #module, strlen(#module)); \
        }                                                                                          \
    })

#endif