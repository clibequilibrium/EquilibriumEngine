#define CR_HOST
// #define CR_DEBUG
#include <cr.h>
#include <engine.h>
#include <world.h>

int main(int argc, char *argv[]) {
    cr_plugin application, editor, engine_simulation;

    // Create an engine instance
    engine_t engine = engine_init(8, true);
    world_t *world  = (world_t *)engine.world;

    application.userdata       = world;
    editor.userdata            = world;
    engine_simulation.userdata = &engine;

    cr_plugin_open(application, CR_PLUGIN("sandbox"));
    cr_plugin_open(editor, CR_PLUGIN("editor"));
    cr_plugin_open(engine_simulation, CR_PLUGIN("engine-simulation"));

    while (engine.running) {
        if (cr_plugin_update(editor) == 0 && cr_plugin_update(application) == 0) {
            cr_plugin_update(engine_simulation);
        }
        cr_plat_init();
    }

    // At the end do not forget to cleanup the plugin context
    cr_plugin_close(application);
    cr_plugin_close(editor);
    cr_plugin_close(engine_simulation);

    return 0;
}