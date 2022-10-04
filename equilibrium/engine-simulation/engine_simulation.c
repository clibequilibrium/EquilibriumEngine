#include <equilibrium.h>
#include <cr.h>

CR_EXPORT int cr_main(struct cr_plugin *ctx, enum cr_op operation) {
    assert(ctx);

    if (operation == CR_STEP) {
        engine_update((engine_t *)ctx->userdata);
    }

    return 0;
}