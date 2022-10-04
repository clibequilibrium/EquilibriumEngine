
#include <SDL_syswm.h>

#include "components/gui.h"
#include "components/renderer/renderer_components.h"
#include "components/sdl_window.h"
#include "bgfx_components.h"
#include "bgfx_system.h"
#include "systems/rendering/gfx_resource_system.h"

ECS_DTOR(Bgfx, ptr, {
    bgfx_shutdown();
    ecs_trace("BGFX successfully shutdown.");
})

static int SetPlatformData(SDL_Window *_window, bgfx_init_t *init) {
    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(_window, &wmi)) {
        return false;
    }

    bgfx_platform_data_t pd;
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    pd.ndt = wmi.info.x11.display;
    pd.nwh = (void *)(uintptr_t)wmi.info.x11.window;
#elif BX_PLATFORM_OSX
    pd.ndt = NULL;
    pd.nwh = wmi.info.cocoa.window;
#elif BX_PLATFORM_WINDOWS
    pd.ndt = NULL;
    pd.nwh = wmi.info.win.window;
#elif BX_PLATFORM_STEAMLINK
    pd.ndt = wmi.info.vivante.display;
    pd.nwh = wmi.info.vivante.window;
#endif // BX_PLATFORM_
    pd.context      = NULL;
    pd.backBuffer   = NULL;
    pd.backBufferDS = NULL;
    bgfx_set_platform_data(&pd);

    init->platformData.nwh = pd.nwh;
    init->platformData.ndt = pd.ndt;

    return true;
}

static void BgfxInitialize(ecs_iter_t *it) {
    Renderer        *renderer          = ecs_field(it, Renderer, 1);
    AppWindow       *app_window        = ecs_field(it, AppWindow, 2);
    AppWindowHandle *app_window_handle = ecs_field(it, AppWindowHandle, 3);

    for (int i = 0; i < it->count; i++) {
        if (!ecs_has(it->world, it->entities[i], SdlWindow)) {
            ecs_err("SdlWindow not found. Please make sure that SdlSystem was "
                    "added.");
            return;
        }

        uint32_t debug = BGFX_DEBUG_PROFILER;
        uint32_t reset = BGFX_RESET_MAXANISOTROPY | BGFX_RESET_MSAA_X16;

        bgfx_init_t init;
        bgfx_init_ctor(&init);

        if (!SetPlatformData(app_window_handle[i].value, &init)) {
            ecs_err("Error: %s", SDL_GetError());
        }

        init.type = (bgfx_renderer_type_t){renderer[i].type};

        bgfx_init(&init);
        bgfx_reset(app_window[i].width, app_window[i].height, reset, init.resolution.format);

        bgfx_set_debug(debug);
        bgfx_set_view_clear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0xFF8C00FF, 1.0f, 0);

        bgfx_set_view_rect(0, 0, 0, (uint16_t)app_window[i].width, (uint16_t)app_window[i].height);

        ecs_set(it->world, it->entities[i], Bgfx, {init, reset});
        renderer[i].type_name = (char *)bgfx_get_renderer_name(bgfx_get_renderer_type());
    }
}

static void BgfxEndRender(ecs_iter_t *it) {
    for (int i = 0; i < it->count; i++) {
        bgfx_frame(false);
    }
}

static void OnAppWindowResized(ecs_iter_t *it) {
    AppWindow *app_window = ecs_field(it, AppWindow, 1);
    Bgfx      *bgfx       = ecs_field(it, Bgfx, 2);

    for (int i = 0; i < it->count; i++) {
        bgfx_reset(app_window[i].width, app_window[i].height, bgfx->reset,
                   bgfx[i].data.resolution.format);
        bgfx_set_view_rect(0, 0, 0, (uint64_t)app_window[i].width, (uint16_t)app_window[i].height);
    }
}

void BgfxSystemImport(world_t *world) {
    ECS_TAG(world, OnEndRender);

    ECS_MODULE(world, BgfxSystem);

    ECS_IMPORT(world, BgfxComponents);
    ECS_IMPORT(world, SdlComponents);
    ECS_IMPORT(world, GuiComponents);
    ECS_IMPORT(world, RendererComponents);
    ECS_IMPORT(world, GfxResourceSystem);

    ecs_set_hooks(world, Bgfx, {.dtor = ecs_dtor(Bgfx)});

    ECS_OBSERVER(world, BgfxInitialize, EcsOnSet, [in] gui.components.Renderer,
                 [in] gui.components.AppWindow, [in] gui.components.AppWindowHandle,
                 !bgfx.components.Bgfx);

    ECS_SYSTEM(world, BgfxEndRender, OnEndRender, [in] bgfx.components.Bgfx);
    ECS_OBSERVER(world, OnAppWindowResized,
                 EcsOnSet, [in] gui.components.AppWindow, [in] bgfx.components.Bgfx);
}