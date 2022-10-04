#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "components/gui.h"
#include "components/input.h"
#include "components/sdl_window.h"

#include "sdl_system.h"

static uint32_t key_sym(uint32_t sdl_sym, bool shift) {
    if (sdl_sym < 128) {
        if (shift) {
            if (sdl_sym == ECS_KEY_EQUALS) {
                sdl_sym = ECS_KEY_PLUS;
            } else if (sdl_sym == ECS_KEY_UNDERSCORE) {
                sdl_sym = ECS_KEY_MINUS;
            } else {
                return sdl_sym;
            }
        }
        return sdl_sym;
    }

    switch (sdl_sym) {
    case SDLK_RIGHT:
        return 'R';
    case SDLK_LEFT:
        return 'L';
    case SDLK_DOWN:
        return 'D';
    case SDLK_UP:
        return 'U';
    case SDLK_LCTRL:
        return 'C';
    case SDLK_LSHIFT:
        return 'S';
    case SDLK_LALT:
        return 'A';
    case SDLK_RCTRL:
        return 'C';
    case SDLK_RSHIFT:
        return 'S';
    case SDLK_RALT:
        return 'A';
    }
    return 0;
}

static void key_down(KeyState *key) {
    if (key->state) {
        key->pressed = false;
    } else {
        key->pressed = true;
    }

    key->state   = true;
    key->current = true;
}

static void key_up(KeyState *key) { key->current = false; }

static void key_reset(KeyState *state) {
    if (!state->current) {
        state->state   = 0;
        state->pressed = 0;
    } else if (state->state) {
        state->pressed = 0;
    }
}

static void mouse_reset(MouseState *state) {
    state->rel.x = 0;
    state->rel.y = 0;

    state->scroll.x = 0;
    state->scroll.y = 0;

    state->view.x = 0;
    state->view.y = 0;

    state->wnd.x = 0;
    state->wnd.y = 0;
}

static void SdlCreateWindow(ecs_iter_t *it) {
    AppWindow   *app_window              = ecs_field(it, AppWindow, 1);
    ecs_entity_t ecs_id(AppWindowHandle) = ecs_field_id(it, 2);

    for (int i = 0; i < it->count; i++) {
        entity_t         entity            = (entity_t){it->entities[i], it->world};
        AppWindowHandle *app_window_handle = entity_get_or_add_component(entity, AppWindowHandle);

        ecs_add(it->world, it->entities[i], SdlWindow);

        const char *title = entity_get_name(entity);
        if (!title) {
            title = "SDL2 window";
        }

        int x = SDL_WINDOWPOS_UNDEFINED;
        int y = SDL_WINDOWPOS_UNDEFINED;

        uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

        if (app_window[i].maximized)
            flags |= SDL_WINDOW_MAXIMIZED;

        SDL_Window *created_window =
            SDL_CreateWindow(title, x, y, app_window[i].width, app_window[i].height, flags);

        if (!created_window) {
            ecs_err("SDL2 window creation failed: %s", SDL_GetError());
            return;
        }

        if (app_window[i].maximized) {
            SDL_SetWindowBordered(created_window, SDL_FALSE);

            int display_index = SDL_GetWindowDisplayIndex(created_window);
            if (display_index < 0) {
                ecs_err("Error getting window display");
                return;
            }

            SDL_Rect usable_bounds;
            if (0 != SDL_GetDisplayUsableBounds(display_index, &usable_bounds)) {
                ecs_err("Error getting usable bounds");
                return;
            }

            SDL_SetWindowPosition(created_window, usable_bounds.x, usable_bounds.y);
            SDL_SetWindowSize(created_window, usable_bounds.w, usable_bounds.h);
        } else {
            SDL_SetWindowSize(created_window, app_window[i].width, app_window[i].height);
        }

        int32_t actual_width, actual_height;
        SDL_GL_GetDrawableSize(created_window, &actual_width, &actual_height);

        app_window_handle->value = created_window;
        app_window[i].width      = actual_width;
        app_window[i].height     = actual_height;
    }
}

static void SdlProcessEvents(ecs_iter_t *it) {
    Input           *input             = ecs_field(it, Input, 1);
    AppWindow       *app_window        = ecs_field(it, AppWindow, 2);
    AppWindowHandle *app_window_handle = ecs_field(it, AppWindowHandle, 3);

    for (int k = 0; k < 128; k++) {
        key_reset(&input->keys[k]);
    }

    key_reset(&input->mouse.left);
    key_reset(&input->mouse.right);

    mouse_reset(&input->mouse);

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (input->callback)
            input->callback(&e);

        if (e.type == SDL_QUIT) {
            world_destroy(it->world);

        } else if (e.type == SDL_KEYDOWN) {
            uint32_t sym = key_sym(e.key.keysym.sym, input->keys['S'].state != 0);
            key_down(&input->keys[sym]);

        } else if (e.type == SDL_KEYUP) {
            uint32_t sym = key_sym(e.key.keysym.sym, input->keys['S'].state != 0);
            key_up(&input->keys[sym]);

        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                key_down(&input->mouse.left);
            } else if (e.button.button == SDL_BUTTON_RIGHT) {
                key_down(&input->mouse.right);
            }

        } else if (e.type == SDL_MOUSEBUTTONUP) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                key_up(&input->mouse.left);
            } else if (e.button.button == SDL_BUTTON_RIGHT) {
                key_up(&input->mouse.right);
            }

        } else if (e.type == SDL_MOUSEMOTION) {
            input->mouse.wnd.x = e.motion.x;
            input->mouse.wnd.y = e.motion.y;
            input->mouse.rel.x = e.motion.xrel;
            input->mouse.rel.y = e.motion.yrel;

        } else if (e.type == SDL_MOUSEWHEEL) {
            input->mouse.scroll.x = e.wheel.x;
            input->mouse.scroll.y = e.wheel.y;
        } else if (e.type == SDL_WINDOWEVENT) {
            if (e.window.event == SDL_WINDOWEVENT_RESIZED ||
                e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {

                for (int i = 0; i < it->count; i++) {
                    if (SDL_GetWindowID(app_window_handle[i].value) == e.window.windowID) {
                        int actual_width, actual_height;

                        SDL_GL_GetDrawableSize(app_window_handle[i].value, &actual_width,
                                               &actual_height);

                        app_window[i].width  = actual_width;
                        app_window[i].height = actual_height;

                        ecs_modified(it->world, it->entities[i], AppWindow);
                        break;
                    }
                }
            }

            if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                world_destroy(it->world);
            }
        }
    }
}

static void SdlDestroyWindow(ecs_iter_t *it) {
    AppWindowHandle *window = ecs_field(it, AppWindowHandle, 1);

    for (int i = 0; i < it->count; i++) {
        SDL_DestroyWindow(window[i].value);
    }
}

void SdlShutdown(world_t *world, void *ctx) {
    SDL_Quit();
    ecs_trace("SDL successfully shutdown.");
}

void SdlSystemImport(world_t *world) {
    ECS_TAG(world, OnInput);

    ECS_MODULE(world, SdlSystem);
    ECS_IMPORT(world, SdlComponents);
    ECS_IMPORT(world, InputComponents);
    ECS_IMPORT(world, GuiComponents);

    ECS_OBSERVER(world, SdlCreateWindow, EcsOnSet, [in] gui.components.AppWindow,
                 !gui.components.AppWindowHandle);

    ECS_SYSTEM(world, SdlProcessEvents, OnInput, input.components.Input($),
               gui.components.AppWindow, gui.components.AppWindowHandle);

    ECS_OBSERVER(world, SdlDestroyWindow, EcsUnSet, [in] gui.components.AppWindowHandle);

    ecs_set_ptr(world, ecs_id(Input), Input, NULL);

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        ecs_err("Unable to initialize SDL: %s", SDL_GetError());
        return;
    }

    SDL_version compiled;
    SDL_version linked;

    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);

    ecs_trace("SDL initialized");
    ecs_trace("Compiled against SDL version %d.%d.%d", compiled.major, compiled.minor,
              compiled.patch);
    ecs_trace("Linked against SDL version %d.%d.%d", linked.major, linked.minor, linked.patch);

    ecs_atfini(world, SdlShutdown, NULL);
}