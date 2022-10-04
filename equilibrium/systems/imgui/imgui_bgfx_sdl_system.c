
#include "cimgui_base.h"
#include "components/input.h"
#include "components/gui.h"
#include "components/sdl_window.h"
#include "components/cglm_components.h"

#include "imgui_bgfx_sdl_system.h"
#include "imgui_bgfx/Roboto_Medium.h"
#include "imgui_bgfx/imgui_impl_bgfx.h"

#include <cimgui_impl.h>

static void ApplyImGuiDarkStyle() {
    ImGuiStyle *style = igGetStyle();

    style->Colors[ImGuiCol_Text]                  = (ImVec4){1.00f, 1.00f, 1.00f, 1.00f};
    style->Colors[ImGuiCol_TextDisabled]          = (ImVec4){0.50f, 0.50f, 0.50f, 1.00f};
    style->Colors[ImGuiCol_WindowBg]              = (ImVec4){0.13f, 0.14f, 0.15f, 1.00f};
    style->Colors[ImGuiCol_ChildBg]               = (ImVec4){0.13f, 0.14f, 0.15f, 1.00f};
    style->Colors[ImGuiCol_PopupBg]               = (ImVec4){0.13f, 0.14f, 0.15f, 1.00f};
    style->Colors[ImGuiCol_Border]                = (ImVec4){0.43f, 0.43f, 0.50f, 0.50f};
    style->Colors[ImGuiCol_BorderShadow]          = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    style->Colors[ImGuiCol_FrameBg]               = (ImVec4){0.25f, 0.25f, 0.25f, 1.00f};
    style->Colors[ImGuiCol_FrameBgHovered]        = (ImVec4){0.38f, 0.38f, 0.38f, 1.00f};
    style->Colors[ImGuiCol_FrameBgActive]         = (ImVec4){0.67f, 0.67f, 0.67f, 0.39f};
    style->Colors[ImGuiCol_TitleBg]               = (ImVec4){0.08f, 0.08f, 0.09f, 1.00f};
    style->Colors[ImGuiCol_TitleBgActive]         = (ImVec4){0.08f, 0.08f, 0.09f, 1.00f};
    style->Colors[ImGuiCol_TitleBgCollapsed]      = (ImVec4){0.00f, 0.00f, 0.00f, 0.51f};
    style->Colors[ImGuiCol_MenuBarBg]             = (ImVec4){0.14f, 0.14f, 0.14f, 1.00f};
    style->Colors[ImGuiCol_ScrollbarBg]           = (ImVec4){0.02f, 0.02f, 0.02f, 0.53f};
    style->Colors[ImGuiCol_ScrollbarGrab]         = (ImVec4){0.31f, 0.31f, 0.31f, 1.00f};
    style->Colors[ImGuiCol_ScrollbarGrabHovered]  = (ImVec4){0.41f, 0.41f, 0.41f, 1.00f};
    style->Colors[ImGuiCol_ScrollbarGrabActive]   = (ImVec4){0.51f, 0.51f, 0.51f, 1.00f};
    style->Colors[ImGuiCol_CheckMark]             = (ImVec4){0.11f, 0.64f, 0.92f, 1.00f};
    style->Colors[ImGuiCol_SliderGrab]            = (ImVec4){0.11f, 0.64f, 0.92f, 1.00f};
    style->Colors[ImGuiCol_SliderGrabActive]      = (ImVec4){0.08f, 0.50f, 0.72f, 1.00f};
    style->Colors[ImGuiCol_Button]                = (ImVec4){0.25f, 0.25f, 0.25f, 1.00f};
    style->Colors[ImGuiCol_ButtonHovered]         = (ImVec4){0.38f, 0.38f, 0.38f, 1.00f};
    style->Colors[ImGuiCol_ButtonActive]          = (ImVec4){0.67f, 0.67f, 0.67f, 0.39f};
    style->Colors[ImGuiCol_Header]                = (ImVec4){0.22f, 0.22f, 0.22f, 1.00f};
    style->Colors[ImGuiCol_HeaderHovered]         = (ImVec4){0.25f, 0.25f, 0.25f, 1.00f};
    style->Colors[ImGuiCol_HeaderActive]          = (ImVec4){0.67f, 0.67f, 0.67f, 0.39f};
    style->Colors[ImGuiCol_Separator]             = style->Colors[ImGuiCol_Border];
    style->Colors[ImGuiCol_SeparatorHovered]      = (ImVec4){0.41f, 0.42f, 0.44f, 1.00f};
    style->Colors[ImGuiCol_SeparatorActive]       = (ImVec4){0.26f, 0.59f, 0.98f, 0.95f};
    style->Colors[ImGuiCol_ResizeGrip]            = (ImVec4){0.00f, 0.00f, 0.00f, 0.00f};
    style->Colors[ImGuiCol_ResizeGripHovered]     = (ImVec4){0.29f, 0.30f, 0.31f, 0.67f};
    style->Colors[ImGuiCol_ResizeGripActive]      = (ImVec4){0.26f, 0.59f, 0.98f, 0.95f};
    style->Colors[ImGuiCol_Tab]                   = (ImVec4){0.08f, 0.08f, 0.09f, 0.83f};
    style->Colors[ImGuiCol_TabHovered]            = (ImVec4){0.33f, 0.34f, 0.36f, 0.83f};
    style->Colors[ImGuiCol_TabActive]             = (ImVec4){0.23f, 0.23f, 0.24f, 1.00f};
    style->Colors[ImGuiCol_TabUnfocused]          = (ImVec4){0.08f, 0.08f, 0.09f, 1.00f};
    style->Colors[ImGuiCol_TabUnfocusedActive]    = (ImVec4){0.13f, 0.14f, 0.15f, 1.00f};
    style->Colors[ImGuiCol_DockingPreview]        = (ImVec4){0.26f, 0.59f, 0.98f, 0.70f};
    style->Colors[ImGuiCol_DockingEmptyBg]        = (ImVec4){0.20f, 0.20f, 0.20f, 1.00f};
    style->Colors[ImGuiCol_PlotLines]             = (ImVec4){0.61f, 0.61f, 0.61f, 1.00f};
    style->Colors[ImGuiCol_PlotLinesHovered]      = (ImVec4){1.00f, 0.43f, 0.35f, 1.00f};
    style->Colors[ImGuiCol_PlotHistogram]         = (ImVec4){0.90f, 0.70f, 0.00f, 1.00f};
    style->Colors[ImGuiCol_PlotHistogramHovered]  = (ImVec4){1.00f, 0.60f, 0.00f, 1.00f};
    style->Colors[ImGuiCol_TextSelectedBg]        = (ImVec4){0.26f, 0.59f, 0.98f, 0.35f};
    style->Colors[ImGuiCol_DragDropTarget]        = (ImVec4){0.11f, 0.64f, 0.92f, 1.00f};
    style->Colors[ImGuiCol_NavHighlight]          = (ImVec4){0.26f, 0.59f, 0.98f, 1.00f};
    style->Colors[ImGuiCol_NavWindowingHighlight] = (ImVec4){1.00f, 1.00f, 1.00f, 0.70f};
    style->Colors[ImGuiCol_NavWindowingDimBg]     = (ImVec4){0.80f, 0.80f, 0.80f, 0.20f};
    style->Colors[ImGuiCol_ModalWindowDimBg]      = (ImVec4){0.80f, 0.80f, 0.80f, 0.35f};
    style->GrabRounding = style->FrameRounding = 2.3f;
}

static void ImguiInitialize(ecs_iter_t *it) {
    Input           *input             = ecs_field(it, Input, 1);
    AppWindowHandle *app_window_handle = ecs_field(it, AppWindowHandle, 2);

    for (int i = 0; i < it->count; i++) {
        if (!ecs_has(it->world, it->entities[i], SdlWindow)) {
            ecs_err("SdlWindow not found. Please make sure that SdlSystem was "
                    "added.");
            return;
        }

        ImGuiContext *context = igCreateContext(NULL);
        ImGuiIO      *io      = igGetIO();

        io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable
        // Gamepad Controls
        io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
        io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport
                                                             // / Platform Windows
        // io.ConfigViewportsNoAutoMerge = true;
        // io.ConfigViewportsNoTaskBarIcon = true;
        // io->ConfigFlags |= ImGuiConfigFlags_IsSRGB;
        // io->ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

        io->BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
        ApplyImGuiDarkStyle();

        ImGui_Implbgfx_Init(255);

        ImFontAtlas_AddFontFromMemoryCompressedTTF(io->Fonts, Roboto_Medium_compressed_data,
                                                   Roboto_Medium_compressed_size, 16, NULL, NULL);

#if BX_PLATFORM_WINDOWS
        ImGui_ImplSDL2_InitForD3D(app_window_handle[i].value);
#elif BX_PLATFORM_OSX
    ImGui_ImplSDL2_InitForMetal(app_window_handle[i].value;
#elif BX_PLATFORM_LINUX
        ImGui_ImplSDL2_InitForOpenGL(app_window_handle[i].value, NULL);
#endif // BX_PLATFORM_WINDOWS ? BX_PLATFORM_OSX ? BX_PLATFORM_LINUX

        input->callback = (_Bool(*)(const void *))ImGui_ImplSDL2_ProcessEvent;
        ecs_set(it->world, it->entities[i], GuiContext, {context});

        const char *version = igGetVersion();
        ecs_trace("ImGui %s initialized", version);
    }
}

static void UpdatePlatformWindows(ImGuiIO *io) {
    // Update and Render additional Platform Windows
    if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        igUpdatePlatformWindows();
        igRenderPlatformWindowsDefault(NULL, NULL);
    }
}

static void ImguiUpdate(ecs_iter_t *it) {
    GuiContext *gui_context = ecs_field(it, GuiContext, 1);

    for (int i = 0; i < it->count; i++) {

        ImGuiContext *context = (ImGuiContext *)gui_context[i].value;
        ImGuiIO      *io      = igGetIO();

        // Error handling
        if (!(context->FrameCount == 0 || context->FrameCountEnded == context->FrameCount)) {

            ImGuiErrorLogCallback callback;
            igErrorCheckEndFrameRecover(NULL, NULL);
            igErrorCheckEndWindowRecover(NULL, NULL);

            igEndFrame();
            UpdatePlatformWindows(io);
        }

        ImGui_Implbgfx_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        igNewFrame();

        ecs_iter_t gui_systems_iterator = ecs_query_iter(it->world, it->ctx);
        while (ecs_query_next(&gui_systems_iterator)) {
            GuiSystem *gui_systems = ecs_field(&gui_systems_iterator, GuiSystem, 1);

            for (int j = 0; j < gui_systems_iterator.count; j++) {
                gui_systems[j].Update(it);
            }
        }

        igRender();
        ImGui_Implbgfx_RenderDrawLists(255, igGetDrawData());

        UpdatePlatformWindows(io);
    }
}

void ImguiShutdown(ecs_iter_t *it) {
    ImGui_ImplSDL2_Shutdown();
    ImGui_Implbgfx_Shutdown();

    ecs_trace("ImGuiBgfx successfully shutdown.");
}

void ImguiBgfxSdlSystemImport(world_t *world) {
    ECS_TAG(world, OnRender);

    ECS_MODULE(world, ImguiBgfxSdlSystem);
    ECS_IMPORT(world, InputComponents);
    ECS_IMPORT(world, GuiComponents);
    ECS_IMPORT(world, SdlComponents);

    ECS_OBSERVER(world, ImguiInitialize, EcsOnSet, input.components.Input($),
                 [in] gui.components.AppWindowHandle, [in] gui.components.Renderer);

    ECS_SYSTEM(world, ImguiUpdate, EcsOnUpdate, [in] gui.components.GuiContext);
    ecs_system(world, {.entity = ImguiUpdate, .ctx = ecs_query_new(world, "GuiSystem")});

    ECS_OBSERVER(world, ImguiShutdown, EcsUnSet, [in] gui.components.GuiContext);
}