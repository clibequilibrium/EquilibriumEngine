#include <equilibrium.h>
#include "bootstrap_system.h"
#include "flecs.h"

static bool CR_STATE initialized = false;

static void import_hot_reloadable_systems(struct cr_plugin *ctx) {
    ECS_IMPORT_HOT_RELOADABLE(ctx, BootstrapSystem);
}

CR_EXPORT int cr_main(struct cr_plugin *ctx, enum cr_op operation) {
    if (operation == CR_CLOSE)
        return 0;

    assert(ctx);
    world_t *world = ctx->userdata;

    if (!initialized) {
        ECS_IMPORT(world, TransformSystem);
        ECS_IMPORT(world, SdlSystem);
        ECS_IMPORT(world, ForwardRendererSystem);
        ECS_IMPORT(world, SkySystem);
        import_hot_reloadable_systems(ctx);

        // Create your app
        entity_t app = entity_create_empty(world, "Open-World");
        entity_add_component(app, AppWindow, {.maximized = true});
        entity_add_component(app, Renderer, {.type = Direct3D11});
        entity_add_component(app, Camera, {.fov = 73.7397953f, .near = 0.1f, .far = 2000.0f});

        initialized = true;
    } else {
        CHECK_IF_PLUGIN_RELOADED(ctx, operation);
        import_hot_reloadable_systems(ctx);
    }

    return 0;
}