// dear imgui: Platform Backend for SDL2
// This needs to be used along with a Renderer (e.g. DirectX11, OpenGL3,
// Vulkan..) (Info: SDL2 is a cross-platform general purpose library for
// handling windows, inputs, graphics context creation, etc.)

// Implemented features:
//  [X] Platform: Mouse cursor shape and visibility. Disable with
//  'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange'. [X] Platform:
//  Clipboard support. [X] Platform: Keyboard arrays indexed using
//  SDL_SCANCODE_* codes, e.g. ImGui::IsKeyPressed(SDL_SCANCODE_SPACE). [X]
//  Platform: Gamepad support. Enabled with 'io.ConfigFlags |=
//  ImGuiConfigFlags_NavEnableGamepad'. [X] Platform: Multi-viewport support
//  (multiple windows). Enable with 'io.ConfigFlags |=
//  ImGuiConfigFlags_ViewportsEnable'.
// Missing features:
//  [ ] Platform: SDL2 handling of IME under Windows appears to be broken and it
//  explicitly disable the regular Windows IME. You can restore Windows IME by
//  compiling SDL with SDL_DISABLE_WINDOWS_IME. [ ] Platform: Multi-viewport +
//  Minimized windows seems to break mouse wheel events (at least under
//  Windows).

// You can use unmodified imgui_impl_* files in your project. See examples/
// folder for examples of using this. Prefer including the entire imgui/
// repository into your project (either as a copy or as a submodule), and only
// build the backends you need. If you are new to Dear ImGui, read documentation
// from the docs/ folder + read the top of imgui.cpp. Read online:
// https://github.com/ocornut/imgui/tree/master/docs

#ifndef IMGUI_IMPL_BGFX_H
#define IMGUI_IMPL_BGFX_H

#include "bgfx_components.h"

BGFX_C_API void ImGui_Implbgfx_Init(int view);
BGFX_C_API void ImGui_Implbgfx_Shutdown();
BGFX_C_API void ImGui_Implbgfx_NewFrame();
BGFX_C_API void ImGui_Implbgfx_RenderDrawLists(bgfx_view_id_t viewId, struct ImDrawData *draw_data);

// Use if you want to reset your rendering device without losing ImGui state.
BGFX_C_API void ImGui_Implbgfx_InvalidateDeviceObjects();
BGFX_C_API bool ImGui_Implbgfx_CreateDeviceObjects();

#endif