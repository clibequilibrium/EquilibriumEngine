#include <equilibrium.h>
#include "cr.h"
#include "editor.h"
#include "flecs.h"

static bool CR_STATE         initialized = false;
static ecs_query_t CR_STATE *gui_systems_query = NULL;

static void custom_cleanup(world_t *world) {
    if (gui_systems_query)
        ecs_query_fini(gui_systems_query);

    gui_systems_query =
        ecs_query_init(world, &(ecs_query_desc_t){.filter.terms = {{.id = ecs_id(GuiSystem)}}});
    ecs_iter_t iterator = ecs_query_iter(world, gui_systems_query);

    while (ecs_query_next(&iterator)) {
        for (int i = 0; i < iterator.count; i++) {
            ecs_delete(world, iterator.entities[i]);
        }
    }
}

CR_EXPORT int cr_main(struct cr_plugin *ctx, enum cr_op operation) {
    if (operation == CR_CLOSE)
        return 0;

    assert(ctx);
    ecs_world_t *world = ctx->userdata;

    if (initialized) {
        CHECK_IF_PLUGIN_RELOADED_W_ACTION(ctx, operation, custom_cleanup);
    }

    // Add systems and components as modules
    ECS_IMPORT_HOT_RELOADABLE(ctx, ImguiBgfxSdlSystem);
    ECS_IMPORT_HOT_RELOADABLE(ctx, ImguiDockspaceSystem);
    ECS_IMPORT_HOT_RELOADABLE(ctx, ImguiOverlaySystem);
    ECS_IMPORT_HOT_RELOADABLE(ctx, ImguiGizmoSystem);

    // ECS_IMPORT_HOT_RELOADABLE(ctx, ImguiEntityInspector);

    initialized = true;
    return 0;
}